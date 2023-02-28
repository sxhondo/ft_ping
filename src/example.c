#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <netinet/in.h>

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
    hints.ai_socktype = SOCK_DGRAM; // SOCK_DGRAM UDP, SOCK_STREAM TCP, SOCK_RAW - ICMP and others protocols 
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
        // int pton = inet_pton(AF_INET, "192.168.1.1", &(sock_in.sin_addr));
        printf(" %s: %s\n", ipver, ipstr);
    }
    freeaddrinfo(res);
    exit(0);
}