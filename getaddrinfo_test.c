#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>


const char* family2Txt(int fam) {
    switch(fam){
        case (AF_INET):
            return "AF_INET";
        case AF_INET6:
            return "AF_INET6";
        case AF_UNSPEC:
            return "AF_UNSPEC";
        default:
            return "NONE";
    }
}


const char* socktype2Txt(int st) {
    switch(st) {
        case SOCK_STREAM:
            return "SOCK_STREAM";
        case SOCK_DGRAM:
            return "SOCK_DGRAM";
        case SOCK_RAW:
            return "SOCK_RAW";
        default:
            return "NONE";
    }
}


const char* protocol2Txt(int proto) {
    static char nm[40];
    bzero(nm, sizeof(nm));
    struct protoent* pnt = getprotobynumber(proto);
    strncpy(nm, pnt->p_name, 40);
    return nm;
}


int main() {

    char nameBuff[40];
    if (gethostname(nameBuff, sizeof(nameBuff)) == 0 ) {
        printf("hostname: %s\n", nameBuff);
        printf ("-----------------------------\n");
        
        struct addrinfo *res=NULL;
        if (getaddrinfo(nameBuff,NULL, NULL, &res)==0) {
            for (;res; res = res->ai_next) {
                printf ("flags: %d\n", res->ai_flags);
                printf ("family: %d: %s\n", res->ai_family, 
                        family2Txt(res->ai_family));
                printf ("socktype: %d: %s\n", res->ai_socktype,
                       socktype2Txt(res->ai_socktype));
                printf ("protocol: %d: %s\n", res->ai_protocol,
                        protocol2Txt(res->ai_protocol));

                printf ("addrlen: %d\n", (int)res->ai_addrlen);

                struct sockaddr_in *ipv4 = 
                    (struct sockaddr_in *)res->ai_addr;
                char ipAddress[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(ipv4->sin_addr), 
                        ipAddress, INET_ADDRSTRLEN);
                printf ("addr: %s\n", ipAddress);
                printf ("canonname: %s\n", res->ai_canonname);
                printf ("-----------------------------\n");
            }
        }
        freeaddrinfo(res);
    }
}
