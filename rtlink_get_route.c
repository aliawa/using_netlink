

// Minimum include set for RTNETLINK
#include <bits/sockaddr.h>
#include <asm/types.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

// Other includes
#include <strings.h>
#include <string.h>     // for memcpy()
#include <stdio.h>
#include <sys/types.h>  // for getpid()
#include <unistd.h>     // for getpid()
#include <arpa/inet.h>  // for inet_ntop()
#include <errno.h>      // for errno
#include <net/if.h>     // for if_indextoname


#define NLMSG_TAIL(nmsg) \
	((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

// A request or a response of RTNETLINK consists of a stream of 
// message structures.
struct Req_t {
    struct nlmsghdr nl;         // RTNETLINK header: this header is required in every request
    struct rtmsg    rt;         // RTNETLINK operaton header: for routing table queries
    char            buf[8192];
};


typedef struct
{
	unsigned char family;
	unsigned char bytelen;
	short bitlen;
	unsigned data[8];
} InetAddr;


char* nlMsgType2str(struct nlmsghdr* hdr) {
    switch (hdr->nlmsg_type) {
        case NLMSG_NOOP   : return "NLMSG_NOOP";
        case NLMSG_ERROR  : return "NLMSG_ERROR";
        case NLMSG_DONE   : return "NLMSG_DONE";
        case NLMSG_OVERRUN: return "NLMSG_OVERRUN";

        case RTM_NEWLINK  : return "RTM_NEWLINK";	
        case RTM_DELLINK  : return "RTM_DELLINK";
        case RTM_GETLINK  : return "RTM_GETLINK";
        case RTM_SETLINK  : return "RTM_SETLINK";

        case RTM_NEWADDR  : return "RTM_NEWADDR";	
        case RTM_DELADDR  : return "RTM_DELADDR";
        case RTM_GETADDR  : return "RTM_GETADDR";

        case RTM_NEWROUTE : return "RTM_NEWROUTE";	
        case RTM_DELROUTE : return "RTM_DELROUTE";
        case RTM_GETROUTE : return "RTM_GETROUTE";

        case RTM_NEWNEIGH : return "RTM_NEWNEIGH";	
        case RTM_DELNEIGH : return "RTM_DELNEIGH";
        case RTM_GETNEIGH : return "RTM_GETNEIGH";

        case RTM_NEWRULE  : return "RTM_NEWRULE";	
        case RTM_DELRULE  : return "RTM_DELRULE";
        case RTM_GETRULE  : return "RTM_GETRULE";

        case RTM_NEWQDISC : return "RTM_NEWQDISC";	
        case RTM_DELQDISC : return "RTM_DELQDISC";
        case RTM_GETQDISC : return "RTM_GETQDISC";

        case RTM_NEWTCLASS  : return "RTM_NEWTCLASS";	
        case RTM_DELTCLASS  : return "RTM_DELTCLASS";
        case RTM_GETTCLASS  : return "RTM_GETTCLASS";

        case RTM_NEWTFILTER : return "RTM_NEWTFILTER";	
        case RTM_DELTFILTER : return "RTM_DELTFILTER";
        case RTM_GETTFILTER : return "RTM_GETTFILTER";

        case RTM_NEWACTION  : return "RTM_NEWACTION";	
        case RTM_DELACTION  : return "RTM_DELACTION";
        case RTM_GETACTION  : return "RTM_GETACTION";

        case RTM_NEWPREFIX  : return "RTM_NEWPREFIX";	

        case RTM_GETMULTICAST : return "RTM_GETMULTICAST"; 

        case RTM_GETANYCAST : return "RTM_GETANYCAST";	

        case RTM_NEWNEIGHTBL : return "RTM_NEWNEIGHTBL";	
        case RTM_GETNEIGHTBL : return "RTM_GETNEIGHTBL";	
        case RTM_SETNEIGHTBL : return "RTM_SETNEIGHTBL";

        case RTM_NEWNDUSEROPT : return "RTM_NEWNDUSEROPT"; 

        case RTM_NEWADDRLABEL : return "RTM_NEWADDRLABEL"; 
        case RTM_DELADDRLABEL : return "RTM_DELADDRLABEL";
        case RTM_GETADDRLABEL : return "RTM_GETADDRLABEL";

        case RTM_GETDCB : return "RTM_GETDCB"; 
        case RTM_SETDCB : return "RTM_SETDCB";

        case RTM_NEWNETCONF : return "RTM_NEWNETCONF"; 
        case RTM_GETNETCONF : return "RTM_GETNETCONF"; 

        case RTM_NEWMDB : return "RTM_NEWMDB"; 
        case RTM_DELMDB : return "RTM_DELMDB"; 
        case RTM_GETMDB : return "RTM_GETMDB"; 
        default: return "UNKNOWN";
    }
}


int str2Inet(InetAddr *addr, const char *name, int family)
{
    memset(addr, 0, sizeof(*addr));

    if (strchr(name, ':')) {
        addr->family = AF_INET6;
        if (inet_pton(AF_INET6, name, addr->data) <= 0)
            return -1;
        addr->bytelen = 16;
        addr->bitlen = 128;
    } else {
        addr->family = AF_INET;
        if (inet_pton(AF_INET, name, addr->data) <= 0)
            return -1;
        addr->bytelen = 4;
        addr->bitlen = 32;
    }
    return 0;
}





int parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
    memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
    while (RTA_OK(rta, len)) {
        unsigned short type = rta->rta_type;
        if ((type <= max) && (!tb[type])){
            tb[type] = rta;
        }
        rta = RTA_NEXT(rta,len);
    }
    if (len) {
        fprintf(stderr, "!!!Deficit %d, rta_len=%d\n", len, rta->rta_len);
    }
    return 0;
}


int addattr_l(struct nlmsghdr *n, int maxlen, int type, const void *data,
        int alen)
{
    int len = RTA_LENGTH(alen);
    struct rtattr *rta;

    if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > maxlen) {
        fprintf(stderr, "addattr_l ERROR: message exceeded bound of %d\n",maxlen);
        return -1;
    }
    rta = NLMSG_TAIL(n); // brings us to the buffer at the end of Req_t
    rta->rta_type = type;
    rta->rta_len = len;
    memcpy(RTA_DATA(rta), data, alen); // brings us to the end of the attribute

    // new length after adding the attribute
    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);
    return 0;
}

void form_request(struct Req_t* pReq, char* ipAdr) {

    // initialize the request buffer
    bzero(pReq, sizeof(struct Req_t));

    // set the NETLINK header
    pReq->nl.nlmsg_len   = NLMSG_LENGTH(sizeof(struct rtmsg));
    pReq->nl.nlmsg_flags = NLM_F_REQUEST;
    pReq->nl.nlmsg_type  = RTM_GETROUTE;

    InetAddr addr;
    str2Inet(&addr, ipAdr, AF_UNSPEC);

    // set the routing message header
    pReq->rt.rtm_table   = RT_TABLE_MAIN;
    pReq->rt.rtm_family  = addr.family;
    pReq->rt.rtm_dst_len = addr.bitlen;

    if (addr.bytelen) {
        addattr_l(&pReq->nl, sizeof(struct Req_t), RTA_DST, 
                &addr.data, addr.bytelen);
    }
}


// send request to kernel
// returns a negative value to indicate error
int send_request(int fd, struct nlmsghdr* pNlmsg) { //struct Req_t* pReq) {

    printf("send_request (fd:%d)\n", fd);
    // create the destination address for the message
    // since it is going to kernel, all we need to populate
    // is the family
    struct sockaddr_nl dstAddr;
    bzero(&dstAddr, sizeof(dstAddr));
    dstAddr.nl_family = AF_NETLINK;

    // The actual RTNETLINK request is placed in 'struct iovec'
    // We can place multiple iovec's in the message.
    struct iovec iov;
    iov.iov_base = (void *) pNlmsg; //&(pReq->nl);
    iov.iov_len  = pNlmsg->nlmsg_len; //pReq->nl.nlmsg_len;

    // Now we create the final message using 'struct msghdr', 
    // that will be sent to the kernel 
    struct msghdr msg;
    bzero(&msg, sizeof(msg));
    msg.msg_name    = (void *) &dstAddr;
    msg.msg_namelen = sizeof(dstAddr);
    msg.msg_iov     = &iov;
    msg.msg_iovlen  = 1;                 // only one iovec was attached

    printf("sending request to kernel on fd:%d\n", fd);
    // send the RTNETLINK message to kernel
    return sendmsg(fd, &msg, 0);
}


// return 
// -1   = error
//  0   = not possible
//  > 0 = bytes received.
int recv_reply(int fd,  unsigned int groups, char* buf, int bufSz)
{
    // initialize the socket read buffer
    bzero(buf, sizeof(buf));

    // read from the socket until the NLMSG_DONE is
    // returned in the type of the bytesRcvdETLINK message
    // or if it was a monitoring socket
    while(1) {
        printf("receiving on descriptor:%d recv buffer size:%d\n", fd, bufSz);
        int bytesRcvd = recv(fd, buf, bufSz, 0);
        printf("received response from kernel: type:%s size:%d bytes \n", 
                nlMsgType2str((struct nlmsghdr *)buf),
                bytesRcvd);

        if (bytesRcvd < 0) {
            if (errno == EINTR || errno == EAGAIN){
                printf("EAGAIN\n");
                continue;
            }
            fprintf(stderr, "netlink receive error %s (%d)\n",
                    strerror(errno), errno);
            return -1;
        }
        if (bytesRcvd == 0) {
            fprintf(stderr, "EOF on netlink\n");
            return -1;
        }

        struct nlmsghdr* pNlmsg = (struct nlmsghdr *)buf;
        int len   = pNlmsg->nlmsg_len;
        int lData = len - sizeof(*pNlmsg);

        if ( lData < 0 || len > bytesRcvd) {
            fprintf(stderr, "!!!malformed message: len=%d\n", len);
            return -1;
        }

        if (pNlmsg->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(pNlmsg);
            if (lData < sizeof(struct nlmsgerr)) {
                fprintf(stderr, "ERROR truncated\n");
            } else {
                if (!err->error) {
                    return bytesRcvd;
                }
                fprintf(stderr, "RTNETLINK answers: %s\n", strerror(-err->error));
                errno = -err->error;
            }
            return -1;
        }
        return bytesRcvd;
    }
}


void read_reply(struct nlmsghdr *nlHdr)
{
    printf("read_reply()\n");

    if (nlHdr->nlmsg_type != RTM_NEWROUTE && nlHdr->nlmsg_type != RTM_DELROUTE) {
        fprintf(stderr, "Not a route: %08x %08x %08x\nlHdr",
                nlHdr->nlmsg_len, nlHdr->nlmsg_type, nlHdr->nlmsg_flags);
        return;
    }

    struct rtmsg *pRtMsg = (struct rtmsg *) NLMSG_DATA(nlHdr);

    int len = nlHdr->nlmsg_len;
    len -= NLMSG_LENGTH(sizeof(*pRtMsg));
    if (len < 0) {
        fprintf(stderr, "BUG: wrong nlmsg len %d\nlHdr", len);
        return;
    }

    struct rtattr* tb[RTA_MAX+1];
    parse_rtattr(tb, RTA_MAX, RTM_RTA(pRtMsg), len);


    char abuf[256];

    if (tb[RTA_DST]) {
        printf("  dst:%s/%u\n", inet_ntop(pRtMsg->rtm_family,
                    RTA_DATA(tb[RTA_DST]),
                    abuf, sizeof(abuf)),
                pRtMsg->rtm_dst_len
              );
    } 

    if (tb[RTA_SRC]) {
        printf("  src:%s/%u\n", inet_ntop(pRtMsg->rtm_family,
                    RTA_DATA(tb[RTA_SRC]),
                    abuf, sizeof(abuf)),
                pRtMsg->rtm_src_len
              );
    }

    if (tb[RTA_OIF]) {
        char buff[IF_NAMESIZE];
        printf("  dev:%s\n", if_indextoname(*(int*)RTA_DATA(tb[RTA_OIF]), buff));
    }

    if (tb[RTA_PREFSRC] ) {
        printf("  prefsrc:%s\n",
                inet_ntop(pRtMsg->rtm_family,
                    RTA_DATA(tb[RTA_PREFSRC]),
                    abuf, sizeof(abuf)));
    }

    if (tb[RTA_IIF] ) {
        printf("  family: %d\n", *(int*)RTA_DATA(tb[RTA_IIF]));
    }

}




// Open netlink socket
int open_netlink(int protocol, int group)
{
    // open netlink socket
    int fd = socket(AF_NETLINK, SOCK_RAW, protocol);

    // fd < 0 means there was en error
    if (fd >=0) {
        // bind the socket to local address (PID)
        struct sockaddr_nl la;
        bzero((void *)&la, sizeof(la));

        la.nl_family = AF_NETLINK;
        la.nl_pid    = getpid();
        la.nl_groups = group;     //multicast group if applicable

        if (bind(fd,(struct sockaddr *)&la,sizeof(la)) < 0) {
            close(fd);
            fd = -1;
        }
    }
    return fd;
}




int main(int argc, char* argv[]) {
    int fd = open_netlink(NETLINK_ROUTE, 0);
    if (fd > 0) {
        // sub functions to create RTNETLINK message,
        // send over socket, receive reply & process
        // message
        struct Req_t req;
        form_request(&req, argv[1]);
        if (send_request(fd, &req.nl) > 0) { //&req) > 0 ) {
            char buf[8192];
            int respLen = recv_reply(fd, 0, buf, sizeof(buf));
            if (respLen > 0 ) {
                read_reply( (struct nlmsghdr *)buf);
            }
        } else {
            printf("send_request failed\n");
        }

        // close socket
        close(fd);
        }
        return 0;
    }


