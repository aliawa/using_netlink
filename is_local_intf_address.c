#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>


void print_address(struct nl_addr* addr) {
    printf("family:%d, len:%d, prefixlen:%d\n",
            nl_addr_get_family(addr), 
            nl_addr_get_len(addr),
            nl_addr_get_prefixlen(addr));
}

struct Address {
    struct rtnl_addr* m_obj;
};

void adr_srch(struct nl_object *obj, void *data)
{
    ((struct Address*)data)->m_obj= (struct rtnl_addr*)obj;
}


static void print_usage(void)
{
	printf("Usage: is_local_intf_address <addr>\n");
	exit(1);
}


void main(int argc, char** argv) {
    struct nl_handle *handle = nl_handle_alloc();
    nl_connect(handle, NETLINK_ROUTE);
    struct nl_cache *link_cache = rtnl_link_alloc_cache(handle);
    struct nl_cache *addr_cache = rtnl_addr_alloc_cache(handle);

	struct nl_addr *addr = NULL;
	if (argc < 2 || !strcmp(argv[1], "-h")){
		print_usage();
    } else {
        addr = nl_addr_parse(argv[1], AF_UNSPEC);
        if (!addr) {
            return;
        }
    }

    print_address(addr);

    // set filter
    struct rtnl_addr *filter = rtnl_addr_alloc();
    rtnl_addr_set_local(filter, addr);
    rtnl_addr_set_family(filter, nl_addr_get_family(addr));

    // Search
    struct Address adr;
    memset(&adr, 0, sizeof(struct Address));
    nl_cache_foreach_filter(addr_cache, (struct nl_object *) filter,
            adr_srch, &adr);

    // Report
    if (adr.m_obj) {
        printf("address of interface:%d\n", rtnl_addr_get_ifindex(adr.m_obj));
    } else {
        printf("Not a local address\n");
    }

    // Cleanup
    nl_cache_free(addr_cache);
    nl_cache_free(link_cache);
    rtnl_addr_put(filter);
    nl_handle_destroy(handle);
}





