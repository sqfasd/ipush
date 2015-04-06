#ifndef SRC_SESSION_SERVER_H_
#define SRC_SESSION_SERVER_H_

#include "deps/base/scoped_ptr.h"
#include "deps/base/concurrent_queue.h"
#include "src/include_std.h"
#include "src/user.h"
#include "src/user_info.h"
#include "src/channel_info.h"
#include "src/http_query.h"
#include "src/stats_manager.h"
#include "src/typedef.h"
#include "src/evhelper.h"
#include "src/message.h"
#include "src/peer/peer.h"
#include "src/sharding.h"

namespace xcomet {

class Storage;
class SessionServerPrivate;
class SessionServer {
 public:
  SessionServer();
  ~SessionServer();
  void Start();
  void Stop();

  void Connect(struct evhttp_request* req);
  void Pub(struct evhttp_request* req);
  void Disconnect(struct evhttp_request* req);
  void Broadcast(struct evhttp_request* req);
  void Stats(struct evhttp_request* req);
  void Sub(struct evhttp_request* req);
  void Unsub(struct evhttp_request* req);
  void Msg(struct evhttp_request* req);

  void OnTimer();
  void OnUserMessage(const string& uid, shared_ptr<string> message);
  void OnPeerMessage(PeerMessagePtr message);
  void OnUserDisconnect(User* user);

  void RunInNextTick(function<void ()> fn);

 private:
  DISALLOW_COPY_AND_ASSIGN(SessionServer);

  void SetupClientHandler();
  void SetupAdminHandler();
  void SetupEventHandler();

  void OnStart();
  void OnStop();

  bool IsHeartbeatMessage(const string& message);
  bool CheckShard(const string& user);
  int  GetShardId(const string& user);
  void HandleMessage(MessagePtr msg);
  void SendUserMsg(MessagePtr msg, int64 ttl, bool check_shard = true);
  void SendChannelMsg(MessagePtr msg, int64 ttl);

  void SendSave(const string& uid, MessagePtr msg, int64 ttl);
  void DoSendSave(MessagePtr msg, int64 ttl);
  void Subscribe(const string& uid, const string& cid);
  void Unsubscribe(const string& uid, const string& cid);
  void UpdateUserAck(const string& uid, int ack);

  const int timeout_counter_;
  UserMap users_;
  UserInfoMap user_infos_;
  ChannelInfoMap channels_;
  UserCircleQueue timeout_queue_;
  StatsManager stats_;
  base::ConcurrentQueue<function<void ()> > task_queue_;

  scoped_ptr<SessionServerPrivate> p_;
  scoped_ptr<Storage> storage_;

  int peer_id_;
  vector<PeerInfo> peers_;
  scoped_ptr<Peer> cluster_;
  scoped_ptr<Sharding<PeerInfo> > sharding_;
};

}  // namespace xcomet
#endif  // SRC_SESSION_SERVER_H_
