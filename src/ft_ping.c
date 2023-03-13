#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <err.h>
#include <sysexits.h>

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

char    ipstr[INET6_ADDRSTRLEN];
u_char  outpackhdr[IP_MAXPACKET], *outpack;

struct sockaddr_in  *dest;
struct addrinfo     *lookup_res;

void stop_it(void)
{
    finish_up = 1;
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
 *
 *
 */
u_short // 0x00 00 77 e5 | 111011111100101
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

/*
 * pinger --
 *	Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first TIMEVAL_LEN
 * bytes of the data portion are used to hold a UNIX "timeval" struct in
 * host byte-order, to compute the round-trip time.
 */
void pinger(void)
{
    outpack = outpackhdr + sizeof(struct ip);

    struct icmp *icp;

    icp = (struct icmp *)outpack;
    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_seq = htons(ntransmitted);
    icp->icmp_id = getpid();

    icp->icmp_cksum = in_cksum((u_short *)icp, cc);

    int sent = sendto(
        icmp_sock,
        (char *)outpack,
        cc,
        0,
        (struct sockaddr *)dest,
        sizeof(struct sockaddr));
    if (sent <= 0)
        err(EX__BASE, "sendto() failed");
    ntransmitted++;
}

int
main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage: ");
        exit(1);
    }

    dns_lookup(argv[1]);

    // socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP) still produces socket() failed: Permission denied
    // although SOCK_DGRAM does not require root :( probably IPPROTO_ICMP does
    icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (icmp_sock < 0)
        err(EX__BASE, "socket() failed");

    printf("PING %s (%s) %d(%lu) bytes of data.\n",
           "todo",
           ipstr,
           defdatalen,
           defdatalen + sizeof(struct ip) + sizeof(struct icmphdr));

    while (!finish_up)
    {
        pinger();
        usleep(SLEEP_TIME);

        struct sockaddr_in from;
        int fromlen = sizeof(struct sockaddr);
        int recv = recvfrom(
            icmp_sock,
            (char *)outpack,
            cc,
            0,
            (struct sockaddr *)&from,
            &fromlen);
        printf("%d bytes from %s: ", recv, inet_ntoa(*(struct in_addr *)&from.sin_addr.s_addr));
        printf("icmp_seq=%d\n", ntransmitted);
    }

    freeaddrinfo(lookup_res);
    close(icmp_sock);
    return 0;
}