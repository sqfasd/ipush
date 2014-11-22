#ifndef SRC_XCOMET_SERVER_H_
#define SRC_XCOMET_SERVER_H_

#include "src/include_std.h"
#include "base/singleton.h"
#include "src/include_std.h"
#include "src/storage.h"
#include "src/user.h"
#include "src/channel.h"
#include "src/router_proxy.h"

namespace xcomet {

class HttpQuery {
 public:
	HttpQuery(const struct evhttp_request *req) {
		evhttp_parse_query(evhttp_request_get_uri(req), &params);
	}
	~HttpQuery(){
		evhttp_clear_headers(&params);
	}
	int GetInt(const char* key, int default_value) {
		const char* val = evhttp_find_header(&params, key);
		return val ?atoi(val) :default_value;
	}
	const char* GetStr(const char* name, const char* default_value) const {
		const char* val = evhttp_find_header(&params, name);
		return val ?val :default_value;
	}
 private:
	struct evkeyvalq params;
};

class XCometServer {
 public:
  typedef map<string, User*> UserMap;
  typedef map<string, Channel*> ChannelMap;

  static XCometServer& Instance() {
    return *Singleton<XCometServer>::get();
  }

  // sub?uid=123&token=abc&type=1[&timeout=30][&noroute=true]
  void Sub(struct evhttp_request* req) {
    if(evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
      // TODO ReplyError module
      evhttp_send_reply(req, 405, "Method Not Allowed", NULL);
      return;
    }

    HttpQuery query(req);
    string uid = query.GetStr("uid", "");
    if (uid.empty()) {
      evhttp_send_reply(req, 410, "Invalid parameters", NULL);
      return;
    }

    int type = query.GetInt("type", 1);
    // string token = query.GetStr("token", "");
    // TODO check request parameters

    User* user = NULL;
    UserMap::iterator iter = users_.find(uid);
    if (iter != users_.end()) {
      user = iter->second;
      user->SetType(type);
      LOG(INFO) << "user has already connected: " << uid;
      evhttp_send_reply(req, 406, "The user has already connected", NULL);
      return;
    } else {
      user = new User(uid, type, req, *this);
      users_[uid] = user;
      router_.RegisterUser(uid);
    }

    user->Start();
    if (storage_.HasOfflineMessage(uid)) {
      MessageIterator iter = storage_.GetOfflineMessageIterator(uid);
      while (iter.HasNext()) {
        user->Send(iter.Next());
      }
      storage_.RemoveOfflineMessages(uid);
    }
  }

  // /pub?uid=123&content=hello
  // /pub?cid=123&content=hello
  void Pub(struct evhttp_request* req) {
    if(evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
      // TODO ReplyError module
      evhttp_send_reply(req, 405, "Method Not Allowed", NULL);
      return;
    }

    HttpQuery query(req);
    string content = query.GetStr("content", "");
    string uid = query.GetStr("uid", "");

    if (!uid.empty()) {
      LOG(INFO) << "pub to user: " << content;
      UserMap::iterator iter = users_.find(uid);
      if (iter != users_.end()) {
        LOG(INFO) << "send to user: " << iter->second->GetUid();
        iter->second->Send(content);
      } else {
        router_.Redirect(uid, content);
      }
    } else {
      string cid = query.GetStr("cid", "");
      if (cid.empty()) {
        LOG(INFO) << "uid and cid both empty";
        evhttp_send_reply(req, 410, "Invalid parameters", NULL);
        return;
      }
      LOG(INFO) << "pub to channel: " << content;
      ChannelMap::iterator iter = channels_.find(cid);
      if (iter != channels_.end()) {
        iter->second->Broadcast(content);
      } else {
        router_.ChannelBroadcast(cid, content);
      }
    }
    ReplyOK(req);
  }

  // /unsub?uid=123&resource=abc
  void Unsub(struct evhttp_request* req) {
  }

  // /createroom?uid=123
  // return roomid
  void CreateRoom(struct evhttp_request* req) {
  }

  void DestroyRoom(struct evhttp_request* req) {
  }

  // /broadcast?content=hello
  void Broadcast(struct evhttp_request* req) {
  }

  // /join?cid=123&uid=456&resource=work
  void Join(struct evhttp_request* req) {
  }

  // /leave?cid=123&uid=456&resource=work
  void Leave(struct evhttp_request* req) {
  }

  void RemoveUser(const string& uid) {
    LOG(INFO) << "RemoveUser: " << uid;
    users_.erase(uid);
  }
 private:
  XCometServer() : storage_(router_) {}
  ~XCometServer() {}
  void ReplyOK(struct evhttp_request* req) {
    evhttp_add_header(req->output_headers, "Content-Type", "text/javascript; charset=utf-8");
    struct evbuffer *buf = evbuffer_new();
    evbuffer_add_printf(buf, "{\"type\":\"ok\"}\n");
    evhttp_send_reply(req, 200, "OK", buf);
    evbuffer_free(buf);
  }

  UserMap users_;
  ChannelMap channels_;
  RouterProxy router_;
  Storage storage_;

  DISALLOW_COPY_AND_ASSIGN(XCometServer);
  friend struct DefaultSingletonTraits<XCometServer>;
};

}  // namespace xcomet
#endif  // SRC_XCOMET_SERVER_H_
