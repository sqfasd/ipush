#ifndef ROUTER_SERVER_H
#define ROUTER_SERVER_H

#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <iostream>

const size_t MAX_LINE = 16384;
const size_t LISTEN_PORT = 40713;

char rot13_char(char c) {
    /* We don't want to use isalpha here; setting the locale would change
     * which characters are considered alphabetical. */
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
        return c + 13;
    else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
        return c - 13;
    else
        return c;
}

using namespace std;

namespace xcomet {

class RouterServer {
    public:
        RouterServer();
        ~RouterServer();
    public:
        void Start();
    public: // callbacks
        static void AcceptCB(evutil_socket_t listener, short event, void *arg);
        static void ReadCB(struct bufferevent* bev, void *ctx);
        static void ErrorCB(struct bufferevent* bev, short error, void *ctx);
};

} // namespace xcomet


#endif
