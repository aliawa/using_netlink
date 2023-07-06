all: is_local_intf_address

get_ips : get_ips.o
	$(CC) -l nl -o $@ $^

nl-route-get-my : nl-route-get-my.o
	$(CC) -l nl -o $@ $^



get_route : get_route.o
	$(CC) -l nl -o $@ $^


is_local_intf_address : is_local_intf_address.o
	$(CC) -l nl -o $@ $^

print_all_routes : print_all_routes.o
	$(CC) -l nl -o $@ $^
