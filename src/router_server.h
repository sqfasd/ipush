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
 private: // callbacks
  //static void AcceptCB(evutil_socket_t listener, short event, void *arg);
#if 0
  static void ConnCloseCB(struct evhttp_connection * conn, void *ctx);
  static void SubReqDoneCB(struct evhttp_request *req, void * ctx);// this class
  static void SubReqChunkCB(struct evhttp_request *req, void * ctx);// this class 
  static void ReqErrorCB(enum evhttp_request_error err, void * ctx);
  static void PubReqDoneCB(struct evhttp_request* req, void * ctx);
#endif
 private:
  //void MakePubReq(const char* host, const int port, const char* uri);
 private:
  //void MakeOpenConnEvent(const char* host, const int port);
   //void CloseConn();
  void ResetSubClient(size_t id);
  void InitClientIpPorts(const string& ips, const string& ports);
  void InitSubClients();
 private:
  typedef string UserID;
  typedef string SessionServerID;
  map<UserID, SessionServerID> u2sMap;
 private:
  vector<pair<string, size_t> > clientipports_;
  vector<shared_ptr<SessionClient> > subclients_;
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
