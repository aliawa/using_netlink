// libnl headers
#include <stdio.h>
#include <netlink/utils.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/route/rtnl.h>
// BUG: it breaks if you don't include rtnl.h first !!
#include <netlink/route/route.h>
#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
struct nl_addr* in_addr2nl_addr(struct in_addr *addr, uint8_t prefix_sz)
{
    char buf[256];
    sprintf(buf, "%s/%d", (char *)inet_ntoa(*addr), (int)prefix_sz);
    return nl_addr_parse(buf, AF_INET);
}

void print_link(struct nl_object* obj, void *arg)
{
    int *item = (int *)arg;
    struct rtnl_link* link = (struct rtnl_link*)obj;
    printf("Link %d name: %s\n", (*item)++, rtnl_link_get_name(link));
    struct nl_dump_params dp = {
        .dp_type = NL_DUMP_FULL,
        .dp_fd = stdout,
        .dp_dump_msgtype = 1,
    };
    nl_object_dump(obj, &dp);
}

void print_route(struct nl_object* obj, void *arg)
{
    int *item = (int *)arg;
    struct rtnl_route* route = (struct rtnl_route*)obj;
    char buf[128];
    struct nl_addr *addr = rtnl_route_get_dst(route);
    if(addr != NULL && rtnl_route_get_family(route) == 2)
    {
        struct in_addr *inaddr = (struct in_addr*)malloc(sizeof(struct in_addr));
        inaddr = nl_addr_get_binary_addr(addr);
        printf("1: Route %s\n", inet_ntoa(*inaddr));
        printf("2: Route %d \ttable %d\tdst %s\t\tdst len %d family %d\n",
                *item,
                rtnl_route_get_table(route),
                nl_addr2str(addr, buf, sizeof(buf)),
                rtnl_route_get_dst_len(route),
                rtnl_route_get_family(route));
        struct nl_addr *my_nl_addr = in_addr2nl_addr(inaddr, rtnl_route_get_dst_len(route));
        printf("3: Route dst %s\t\tdst len %d\n",
                nl_addr2str(my_nl_addr, buf, sizeof(buf)),
                nl_addr_get_prefixlen(my_nl_addr));
    } else
        printf("Route %d\n", *item);
    (*item)++;
    // struct nl_dump_params dp = {
    // .dp_type = NL_DUMP_FULL,
    // .dp_fd = stdout,
    // .dp_dump_msgtype = 1,
    // };
    //
    // if(rtnl_route_get_table(route) == 254)
    // nl_object_dump(obj, &dp);
    //
    // route_dump_full(route, &params);
}
void print_addr(struct nl_object* obj, void *arg)
{
    int ifindex = *(int*)arg;
    struct rtnl_addr* addr = (struct rtnl_addr*)obj;
    struct nl_dump_params dp = {
        .dp_type = NL_DUMP_FULL,
        .dp_fd = stdout,
        .dp_dump_msgtype = 1,
    };
    nl_object_dump(obj, &dp);
}


void my_call_back(struct nl_object* obj, void *arg) {
    struct nl_addr* dstAdr = (struct nl_addr*)arg;

    struct rtnl_route* route = (struct rtnl_route*)obj;

    struct nl_addr *routeDstAdr = rtnl_route_get_dst(route);
    if(routeDstAdr != NULL ) {
        char buff[100];
        if (nl_addr_cmp(dstAdr, routeDstAdr) == 0 ){
            struct nl_addr *routeSrcAdr = rtnl_route_get_src(route);
            if (routeSrcAdr !=NULL) {
                printf("Use src address: %s\n", nl_addr2str(routeSrcAdr, buff, sizeof(buff)));
            } else {
                printf("Src address is NULL for dst: %s\n", nl_addr2str(dstAdr, buff, sizeof(buff)));
            }
        } else {
            printf("route dst:%s  no match\n", nl_addr2str(routeDstAdr, buff, sizeof(buff)));
        }
    } else {
        printf("route dst is NULL\n");
    }
}
        


void my_print_route(struct nl_object* obj, void *arg)
{
    struct rtnl_route* route = (struct rtnl_route*)obj;

    int table                = rtnl_route_get_table(route);
    int scope                = rtnl_route_get_scope(route);
    int tos                  = rtnl_route_get_tos(route);
    int protocol             = rtnl_route_get_protocol(route);
    struct nl_addr *dst      = rtnl_route_get_dst(route);
    struct nl_addr *src      = rtnl_route_get_src(route);
    int type                 = rtnl_route_get_type(route);
    struct nl_addr *pref_src = rtnl_route_get_pref_src(route);
    char* iif                = rtnl_route_get_iif(route);


    // scope: see /etc/iproute2/rt_scopes

    char buf1[50];
    char buf2[50];
    char buf3[50];
    char buf4[10];
    char buf5[10];
    printf ("%26s %4s %5d %3d %5d %9s %26s %8s %5s\n",
            (dst)? nl_addr2str(dst, buf2, sizeof(buf2)):"NULL",
            (src)? nl_addr2str(src, buf1, sizeof(buf1)):"NULL",
            table, 
            tos,
            protocol,
            nl_rtntype2str(type,buf5,sizeof(buf5)),
            (pref_src)? nl_addr2str(pref_src, buf3, sizeof(buf3)):"NULL",
            rtnl_scope2str(scope, buf4, sizeof(buf4)),
            iif);
}



int main(int argc, char **argv)
{
    struct nl_handle *sock;
    // Allocate a new netlink socket
    sock = nl_handle_alloc();
    // Connect to link netlink socket on kernel side
    nl_connect(sock, NETLINK_ROUTE);

    // Create the route cache
    struct nl_cache *route_cache = rtnl_route_alloc_cache(sock);
    printf("Route cache (%d routes):\n", nl_cache_nitems(route_cache));

    // Print All routes
    printf ("%26s %4s %5s %3s %5s %9s %26s %8s %5s\n",
            "dst",
            "src",
            "table",
            "tos",
            "proto",
            "type",
            "pref_src",
            "scope",
            "iif");
    int item = 0;
    nl_cache_foreach(route_cache, my_print_route, (void *)&item);


    if (argc < 2 ) {
        printf ("Usage %s <dst-ip-address>\n", argv[0]);
        return 1;
    }
    
   
   // Test 1 
//     struct nl_addr* dstAdr = nl_addr_parse(argv[1], AF_INET);
//     nl_cache_foreach(route_cache, my_call_back, dstAdr);

    // Version 2
//     struct nl_addr* dstAdr = nl_addr_parse(argv[1], AF_INET);
//     struct rtnl_addr *filter = rtnl_addr_alloc();
//     nl_cache_foreach_filter(route_cache, (struct nl_object *) filter,
//             my_call_back, dstAdr);

}
