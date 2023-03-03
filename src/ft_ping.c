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

    printf("PING %s (%s) %d(%d) bytes of data.\n", argv[1], ipstr, 1, 1);
    int icmp_sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (icmp_sock < 0)
    {
        perror("ping: icmp open socket");
        exit(2);
    }

    u_char outpackhdr[IP_MAXPACKET], *outpack;
    outpack = outpackhdr + sizeof(struct ip);

    struct icmp *icp;

    icp = (struct icmp *)outpack;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = htons(1);
	icp->icmp_id = getpid();

    int cc = ICMP_MINLEN + 56;

    int sent = sendto(
        icmp_sock,
        &outpack,
        cc,
        0,
        (struct sockaddr *)dest,
        sizeof(dest)
    );

    freeaddrinfo(res);
    close(icmp_sock);
    return 0;
}