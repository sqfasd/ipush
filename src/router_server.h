#ifndef ROUTER_SERVER_H
#define ROUTER_SERVER_H

#include "session_client.h"
#include "deps/base/shared_ptr.h"

#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <map>

#include <iostream>

using namespace std;

namespace xcomet {

using base::shared_ptr;


class RouterServer {
 public:
  RouterServer();
  ~RouterServer();
 public:
  void Start();
  void MakeCliErrEvent(CliErrInfo* clierr);
  void MakePubEvent(size_t clientid, const char* pub_uri);
 private:
  void ResetSubClient(size_t id);
  void InitClientIpPorts(const string& ips, const string& ports);
  void InitSubClients();
 private:
  typedef string UserID;
  typedef string SessionServerID;
  map<UserID, SessionServerID> u2sMap;
 private:
  vector<pair<string, size_t> > clientipports_;
  vector<shared_ptr<SessionClient> > session_clients_;
 private: //callbacks
  static void ClientErrorCB(int sock, short which, void *arg);
 private:
  //void InitConn(const char* host, int port);
  //void ReInitConn();
 private: // socket and libevent
  struct event_base *evbase_;
};

} // namespace xcomet


#endif
