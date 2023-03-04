#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

// struct ping_packet
// {
//     struct icmphdr hdr;
//     char msg[64 - sizeof(struct icmphdr)];
// };

// int ping_loop = 1;

// void interruption_handler()
// {
//     ping_loop = 0;
// }

/*
 * in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 */
u_short
in_cksum(u_short *addr, int len)
{
	int nleft, sum;
	u_short *w;
	union {
		u_short	us;
		u_char	uc[2];
	} last;
	u_short answer;

	nleft = len;
	sum = 0;
	w = addr;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		last.uc[0] = *(u_char *)w;
		last.uc[1] = 0;
		sum += last.us;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}

int 
main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("%s\n", "usage: ");
        exit(0);
    }

    int status;
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;

    if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(2);
    }

    char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in *dest = NULL;
    for (struct addrinfo *p = res; p != NULL; p = p->ai_next)
    {
        if (p->ai_family == AF_INET)
        {
            dest = (struct sockaddr_in *)p->ai_addr;
            inet_ntop(p->ai_family, &(dest->sin_addr), ipstr, sizeof(ipstr));
            break ;
        }
    }

    int icmp_sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (icmp_sock < 0)
    {
        perror("ping: icmp open socket");
        exit(2);
    }


    u_char outpackhdr[IP_MAXPACKET], *outpack;
    outpack = outpackhdr + sizeof(struct ip);

    int defdatalen = 56;
    int cc = ICMP_MINLEN + defdatalen;

    struct icmp *icp;

    icp = (struct icmp *)outpack;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = htons(1);
	icp->icmp_id = getpid();

    icp->icmp_cksum = in_cksum((u_short *)icp, cc);

    printf("PING %s (%s) %d(%lu) bytes of data.\n", 
        argv[1], 
        ipstr, 
        defdatalen, 
        defdatalen + sizeof(struct ip) + sizeof(struct icmphdr)
    );

    int sent = sendto(
        icmp_sock,
        (char *)outpack,
        cc,
        0,
        (struct sockaddr *)dest,
        sizeof(struct sockaddr)
    );
    if (sent <= 0)
    {
        perror("sent failed");
    }
    printf("bytes sent: %d", sent);

    // struct sockaddr_in retaddr; 
    // int recv = recvfrom(
    //     icmp_sock,
    //     (char *)outpack,
    //     cc,
    //     0,
    //     (struct sockaddr *)&retaddr,
    //     sizeof(struct sockaddr)
    // );
    // printf("bytes received: %d", recv);

    freeaddrinfo(res);
    close(icmp_sock);
    return 0;
}