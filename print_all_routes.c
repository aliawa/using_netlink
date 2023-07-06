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
}
