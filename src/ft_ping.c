#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <err.h>
#include <sysexits.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

#define	SLEEP_TIME	1000000

int icmp_sock;
int finish_up;

int defdatalen = 56;
int cc = ICMP_MINLEN + 56;
int ntransmitted;
int nreceived;

char ipstr[INET6_ADDRSTRLEN];

u_long ttl;

/* options */
int options;
#define F_VERBOSE   0x0001
#define F_TTL       0x0002

struct sockaddr_in  *dest;
struct addrinfo     *lookup_res;

struct tv32 {
	u_int32_t sec;
	u_int32_t usec;
};

void
stop_it(int ignored)
{
    finish_up = 1;
}

void
print_hex(void *mem, int len)
{
    puts("=======================================");
    u_char *p = mem;
    for (int i = 0; i < len; i++)
    {
        if (i != 0 && i % 8 == 0)
            putchar('\n');
        printf("%02x%02x ", *p, *(p + 1));
        p += 2;
    }
    puts("\n=======================================\n");
}

/*
 * checksum algorithm for a data segment of 8-bits (1011 0110) separated into 2 4-bit words.
 *  1011
 * +0110 add two 4-bit words
 * -----
 * 10001 perform ones-complement addition
 *     1
 * -----
 * ~0010 flip bits
 * -----
 *  1101 = result checksum
 */
u_short
in_cksum(u_short *addr, int len)
{
    u_int nleft = len;
    u_int sum = 0;
    u_short *w = addr;
    u_short result;

    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    /*
     * mop up an odd byte, if necessary
     */
    union
    {
        u_short us;
        u_char uc[2];
    } last;

    if (nleft == 1)
    {
        last.uc[0] = *(u_char *)w;
        last.uc[1] = 0;
        sum += last.us;
    }

    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);                 /* add carry */
    result = ~sum;                      /* truncate to 16 bits */
    return result;
}

void 
dns_lookup(char dns[])
{
    int status;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;

    if ((status = getaddrinfo(dns, NULL, &hints, &lookup_res)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(2);
    }

    for (struct addrinfo *p = lookup_res; p != NULL; p = p->ai_next)
    {
        if (p->ai_family == AF_INET)
        {
            dest = (struct sockaddr_in *)p->ai_addr;
            inet_ntop(p->ai_family, &(dest->sin_addr), ipstr, sizeof(ipstr));
            break;
        }
    }
}

double
round_trip(struct icmp *icp)
{
    void *tstamp;
    double triptime = 0.0;
    struct timeval now;
    struct tv32 in_tv32;
    struct tv32 diff;
    struct timeval in;

    tstamp = icp->icmp_data;
    memcpy(&in_tv32, tstamp, sizeof(in_tv32));
    in.tv_sec = ntohl(in_tv32.sec);
    in.tv_usec = ntohl(in_tv32.usec);

    gettimeofday(&now, NULL);

    // printf("in: %lu : %lu\n", in.tv_sec, in.tv_usec);
    // printf("now: %lu : %lu\n", now.tv_sec, now.tv_usec);

    diff.usec = now.tv_usec - in.tv_usec;
    if (diff.usec < 0)
    {
        now.tv_sec -= 1;
        now.tv_usec += 1000000;
    }
    diff.sec = now.tv_sec - in.tv_sec;
    // printf("diff: %u : %u\n", diff.sec, diff.usec);

    // printf("diff: %f+%f\n", ((double)diff.sec) * 1000.0, ((double)diff.usec / 1000.0));
    triptime = ((double)diff.sec) * 1000.0 + ((double)diff.usec / 1000.0);
    return triptime;
}

/*
 * pinger --
 *	Compose and transmit an ICMP ECHO REQUEST packet. The IP packet
 * will be added on by the kernel. The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer. The first TIMEVAL_LEN
 * bytes of the data portion are used to hold a UNIX "timeval" struct in
 * host byte-order, to compute the round-trip time.
 */
void
pinger(void)
{
    struct timeval now;
    struct tv32 tv32;
    struct icmp *icp;
    u_char outpackhdr[IP_MAXPACKET], *outpack;

    outpack = outpackhdr + sizeof(struct ip);

    icp = (struct icmp *)outpack;
    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_seq = htons(ntransmitted);
    icp->icmp_id = getpid();

    gettimeofday(&now, NULL);
    tv32.sec = htonl(now.tv_sec);
    tv32.usec = htonl(now.tv_usec);
    memmove(
        (void *)&outpack[ICMP_MINLEN],
        (void *)&tv32,
        sizeof(tv32));

    icp->icmp_cksum = in_cksum((u_short *)icp, cc);

    int sent = sendto(
        icmp_sock,
        outpack,
        cc,
        0,
        (struct sockaddr *)dest,
        sizeof(struct sockaddr));
    if (sent <= 0)
        err(EX__BASE, "sendto() failed");
    ntransmitted++;
}

void
receiver(void)
{
    struct ip *ip;
    struct icmp *icp;
    struct sockaddr_in from;
    u_char inpacket[IP_MAXPACKET], *inpack = inpacket;

    int fromlen = sizeof(struct sockaddr);
    int recv = recvfrom(
        icmp_sock,
        inpack,
        cc,
        0,
        (struct sockaddr *)&from,
        &fromlen);
    if (recv <= 0)
        err(EX__BASE, "recvfrom() failed");

    ip = (struct ip *)inpack;
    icp = (struct icmp *)(inpack + sizeof(struct ip));
    
    if (options & F_VERBOSE)
    {
        print_hex(ip, sizeof(struct ip) + sizeof(struct icmp));
    }

    double triptime = round_trip(icp);
    nreceived++;

    printf("%d bytes from %s:", recv, inet_ntoa(*(struct in_addr *)&from.sin_addr.s_addr));
    printf(" icmp_seq=%d", ntransmitted);
    printf(" ttl=%d", ip->ip_ttl);
    printf(" time=%.3f ms\n", triptime);
}

int
main(int argc, char **argv)
{
    int ch;
    char *tmp;

    dns_lookup(argv[1]);
    signal(SIGINT, stop_it);

    // socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP) still produces socket() failed: Permission denied
    // although SOCK_DGRAM does not require root, probably IPPROTO_ICMP  `
    icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (icmp_sock < 0)
        err(EX__BASE, "socket() failed");

    while ((ch = getopt(argc, argv, "T:t:vV")) != -1)
    {
        switch (ch)
        {
            case 'T':
            case 't':
                ttl = strtol(optarg, &tmp, 0);
                if (ttl > MAXTTL)
                    errx(EX_USAGE, "invalid TTL: %s", optarg);
                options |= F_TTL;
                break;
            case 'V':
            case 'v':
                options |= F_VERBOSE;
                break;
            default:
                break;
        }
    }

    if (options & F_TTL)
    {
        if ((setsockopt(icmp_sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl))) != 0)
            err(EX_OSERR, "setsockopt()");
    }

    printf("PING %s (%s) %d(%lu) bytes of data.\n",
           argv[1],
           ipstr,
           defdatalen,
           defdatalen + sizeof(struct ip) + sizeof(struct icmphdr));

    while (!finish_up)
    {
        pinger();
        receiver();
        usleep(SLEEP_TIME);
        break;
    }

    printf("\n--- %s ping statistics ---\n", argv[1]);
    printf("%d packets transmitted, %d received\n", ntransmitted, nreceived);

    freeaddrinfo(lookup_res);
    close(icmp_sock);
    return 0;
}