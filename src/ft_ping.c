#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

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
    }

    int status;
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;

    printf("performing DNS lookup for %s:\n\n", argv[1]);
    if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(2);
    }

    struct sockaddr_in *dest_sock_in = NULL;
    for (struct addrinfo *p = res; p != NULL; p = p->ai_next)
    {
        if (p->ai_family == AF_INET)
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            char ipstr[INET6_ADDRSTRLEN];
            inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
            printf("using ip: %s\n", ipstr);
            break ;
        }
    }

    // int icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    // if (icmp_sock < 0)
    // {
    //     perror("ping: icmp open socket");
    //     exit(2);
    // }

    // int sent = sendto(
    //     icmp_sock,
    //     &packet,
    //     sizeof(packet),
    //     0,
    //     (struct sockaddr *)sock_in,
    //     sizeof(*sock_in)
    // );

    return 0;
}