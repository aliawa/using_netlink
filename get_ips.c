#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/object-api.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/addr.h>


const char* family2str(int family) {
    switch (family) {
        case AF_INET: return "AF_INET";
        case AF_INET6: return "AF_INET6";
        default: return "NONE";
    }
}


void add_addr_to_array(struct nl_object *obj, void *data)
{
    struct rtnl_addr* addrObj = (struct rtnl_addr *) obj; 
    struct nl_addr *naddr = rtnl_addr_get_local(addrObj);

    int family = nl_addr_get_family(naddr);
    char* label = rtnl_addr_get_label(addrObj);
    int ifindex = rtnl_addr_get_ifindex(addrObj);
    int scope = rtnl_addr_get_scope(addrObj);

    char addrBuff[50];
    nl_addr2str(naddr, addrBuff, sizeof(addrBuff));
    
    char buff[10];
    printf("%s family:%s, label:%s, ifindex:%d, scope:%s\n", 
            addrBuff,
            family2str(family),
            label,
            ifindex,
            rtnl_scope2str(scope, buff, sizeof(buff)));
}


void main() {
    struct nl_handle *handle = nl_handle_alloc();
    nl_connect(handle, NETLINK_ROUTE);
    struct nl_cache *link_cache = rtnl_link_alloc_cache(handle);
    struct nl_cache *addr_cache = rtnl_addr_alloc_cache(handle);

    struct rtnl_addr *filter = rtnl_addr_alloc();
    //rtnl_addr_set_ifindex(filter, rtnl_link_name2i(link_cache, "wlan0"));

    rtnl_addr_set_scope(filter, rtnl_str2scope("universe"));
    //rtnl_addr_set_family(filter, AF_INET6);
    nl_cache_foreach_filter(addr_cache, (struct nl_object *) filter,
            add_addr_to_array, NULL);

    //rtnl_addr_set_family(filter, AF_INET);
    //nl_cache_foreach_filter(addr_cache, (struct nl_object *) filter,
    //        add_addr_to_array, NULL);

    nl_cache_free(addr_cache);
    nl_cache_free(link_cache);
    rtnl_addr_put(filter);
    nl_handle_destroy(handle);
}





