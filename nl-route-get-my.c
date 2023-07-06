/*
 * src/nl-route-get.c     Get Route Attributes
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

#include "utils.h"


extern struct nl_object *nl_cache_search(struct nl_cache *cache,
				  struct nl_object *needle);

static void print_usage(void)
{
	printf("Usage: nl-route-get <addr>\n");
	exit(1);
}

static int cb(struct nl_msg *msg, void *arg)
{
	nl_cache_parse_and_add(arg, msg);

	return 0;
}


void cleanup(struct nl_handle *nlh, 
        struct nl_cache *route_cache,
        struct nl_addr* addr
        ) {
    if (route_cache) {
        nl_cache_free(route_cache);
    }
    if (nlh) {
        nl_close(nlh);
        nl_handle_destroy(nlh);
    }
    if (addr) {
        nl_addr_put(addr);
    }
}

struct Route {
    struct rtnl_route* m_obj;
};


void route_cb(struct nl_object* obj, void *arg) {
    ((struct Route*)arg)->m_obj = (struct rtnl_route*)obj;
}



int init (struct nl_handle **nlh,  struct nl_cache **route_cache) {

	*nlh = nltool_alloc_handle();
	if (!*nlh) {
		return -1;
    }

	if (nltool_connect(*nlh, NETLINK_ROUTE) < 0) {
        cleanup(*nlh, *route_cache, NULL);
		return -1;
    }

	*route_cache = nltool_alloc_route_cache(*nlh);
	if (!*route_cache){
        cleanup(*nlh, *route_cache, NULL);
        return -1;
    }
    return 0;
}



int main(int argc, char *argv[])
{
	struct nl_handle *nlh = NULL;
	struct nl_cache *route_cache = NULL;
	struct nl_addr *dst = NULL;
    
	if (argc < 2 || !strcmp(argv[1], "-h"))
		print_usage();

    if (init(&nlh, &route_cache)==-1) {
        return -1;
    }

	dst = nltool_addr_parse(argv[1]);
	if (!dst) {
        cleanup(nlh, route_cache, dst);
		return -1;
    }

    struct rtnl_route *route = rtnl_route_alloc();
    rtnl_route_set_dst(route, dst);
    struct Route cb_obj;
    memset(&cb_obj, 0, sizeof(struct Route));
    nl_cache_foreach_filter(route_cache, (struct nl_object*)route, route_cb, &cb_obj);

    if (cb_obj.m_obj) {
        printf("Route found in cache\n");
        char buf3[50];
        struct nl_addr *pref_src = rtnl_route_get_pref_src(cb_obj.m_obj);
        printf ("src: %s\n", nl_addr2str(pref_src, buf3, sizeof(buf3)));
    } else {
        printf("Route not in cache. Sending Query to kernel\n");
		struct nl_msg *m;
		struct rtmsg rmsg = {
			.rtm_family = nl_addr_get_family(dst),
			.rtm_dst_len = nl_addr_get_prefixlen(dst),
		};

		m = nlmsg_alloc_simple(RTM_GETROUTE, 0);
		nlmsg_append(m, &rmsg, sizeof(rmsg), NLMSG_ALIGNTO);
		nla_put_addr(m, RTA_DST, dst);

		if (nl_send_auto_complete(nlh, m) < 0) {
			nlmsg_free(m);
			fprintf(stderr, "%s\n", nl_geterror());
            cleanup(nlh, /*link_cache,*/ route_cache, dst);
            return -1;
		}

		nlmsg_free(m);

		nl_socket_modify_cb(nlh, NL_CB_VALID, NL_CB_CUSTOM, cb,
				 route_cache);

		if (nl_recvmsgs_default(nlh) < 0) {
			fprintf(stderr, "%s\n", nl_geterror());
            cleanup(nlh, /*link_cache,*/ route_cache, dst);
            return -1;
		}
        printf("Route Query successful\n");


        nl_cache_foreach_filter(route_cache, (struct nl_object*)route, route_cb, &cb_obj);
        if (cb_obj.m_obj) {
            printf("Route found in cache\n");
            char buf3[50];
            struct nl_addr *pref_src = rtnl_route_get_pref_src(cb_obj.m_obj);
            printf ("src: %s\n", nl_addr2str(pref_src, buf3, sizeof(buf3)));
        } else {
            printf("No route found\n");
        }
    }

	return 0;
}
