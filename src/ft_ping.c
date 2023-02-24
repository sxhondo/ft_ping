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

// int send_probe(int icmp_sock, struct sockaddr_in *sock_in, char *ping_dom, char *ip, char *rev_host)
// {
//     struct msghdr msg;
//     struct ping_packet packet;
//     int msg_count = 0;

//     // while (1)
//     // {
//         memset(&packet, 0, sizeof(struct ping_packet));
//         packet.hdr.type = ICMP_ECHO;
//         packet.hdr.un.echo.id = getpid();

//         int i = 0;
//         for (i = 0; i < sizeof(packet.msg) - 1; i++)
//         {
//             packet.msg[i] = i + '0';
//         }
//         packet.msg[i] = 0;
//         packet.hdr.un.echo.sequence = msg_count++;
//         packet.hdr.checksum = 1;

//         int sent = sendto(
//             icmp_sock, 
//             &packet, 
//             sizeof(packet), 
//             0, 
//             (struct sockaddr *)sock_in, 
//             sizeof(*sock_in)
//         );
//         if (sent < 0)
//         {
//             printf("sent failed");
//         }
//         else
//         {
//             printf("sent: %d\n", sent);
//         }
//     // }
// }

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("%s\n", "usage: ");
    }

    int status;
    struct addrinfo hints;
    struct addrinfo *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // AF_UNSPEC for any
    hints.ai_socktype = SOCK_STREAM; // SOCK_DGRAM for UDP, SOCK_RAW - for others
    // hints.ai_flags = AI_PASSIVE; // use my ip (insted pass first paremeter to getaddrinfo)

    /* 
        int
        getaddrinfo(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res);

        The getaddrinfo() function is used to get a list of IP addresses and port numbers for host hostname and service servname. 
        It is a replacement for and provides more flexibility than the gethostbyname(3) and getservbyname(3) functions.
        NULL - could be address specified by user
        hints - hints filled above and as for man() "hints is an optional pointer to a struct addrinfo, as defined by ⟨netdb.h⟩"
        res - result filled by getaddrinfo()
    */
    if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(2);
    }

    printf("DNS lookup for %s:\n\n", argv[1]);
    char ipstr[INET6_ADDRSTRLEN];
    for (struct addrinfo *p = res; p != NULL; p = p->ai_next)
    {
        void *addr;
        char *ipver;
        if (p->ai_family == AF_INET)
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } 
        else 
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }
        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        printf(" %s: %s\n", ipver, ipstr);
    }
    freeaddrinfo(res);
    exit(0);

    // int icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    // if (icmp_sock < 0)
    // {
    //     perror("ping: icmp open socket");
    //     exit(2);
    // }


    // // catching interrupt
    // signal(SIGINT, interruption_handler);

    // // struct sockaddr sock_addr;
    // struct sockaddr_in sock_in;

    // // presentation to network
    // int pton = inet_pton(AF_INET, "192.168.1.1", &(sock_in.sin_addr));
    //  // IPv4 - ntop example
    //     // char ip4[INET_ADDRSTRLEN]; // buffer for IPv4 address
    //     // struct sockaddr_in sa; // filled by something
    //     // inet_ntop(AF_INET, &(sa.sin_addr), ip4, INET_ADDRSTRLEN);
    //     // printf(“The IPv4 address is: %s\n”, ip4);
    // if (pton < 0) 
    // {
    //     perror("ping: pton failure");
    //     exit(2);
    // }
    // send_probe(icmp_sock, &sock_in, NULL, "ip_addr", NULL);
    // return 0;
}