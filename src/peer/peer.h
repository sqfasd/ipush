#ifndef SRC_PEER_PEER_H_
#define SRC_PEER_PEER_H_

#include "deps/base/concurrent_queue.h"
#include "src/include_std.h"

using std::string;
using std::vector;
using std::shared_ptr;

struct PeerInfo {
  int id;
  string ip;
  string public_addr;
  string admin_addr;
};

const int PMT_REDIRECT_TO_SERVER = 1;
const int PMT_NOTIFY_TO_USER = 2;

struct PeerMessage {
  int source;
  int target;

  // following is message frame data
  int type;
  // if type is PMT_REDIRECT_TO_SERVER, user refers to where the msg come from
  // maybe the endpoint user or the backend service
  // if type is PMT_NOTIFY_TO_USER, user refers to the endpoint where we send
  // the message to
  string user;
  string content;
};

inline ostream& operator<<(ostream& os, const PeerMessage& msg) {
  os << "PeerMessage(" << msg.source
     << ", " << msg.target
     << ", " << msg.type
     << ", " << msg.user
     << ", " << msg.content << ")";
}

typedef shared_ptr<PeerMessage> PeerMessagePtr;
typedef function<void (PeerMessagePtr)> PeerMessageCallback;

class Peer {
 public:
  Peer(const int id, const vector<PeerInfo>& peers);
  ~Peer();
  void Start();
  void Stop();
  void Restart();
  void Broadcast(const int type, const string& user, string& content);
  void Broadcast(const int type, const string& user, const char* content);
  void Send(const int target,
            const int type,
            const string& user,
            string& content);
  void Send(const int target,
            const int type,
            const string& user,
            const char* content);
  void SetMessageCallback(const PeerMessageCallback& cb) {msg_cb_ = cb;}

 private:
  void Sending();
  void Receiving();
  void StartSend();
  void StartReceive();
  void StopSend();
  void StopReceive();
  void RestartSend();
  void RestartReceive();

  const int id_;
  vector<PeerInfo> peers_;
  base::ConcurrentQueue<PeerMessagePtr> outbox_;
  std::atomic<bool> s_stoped_;
  std::atomic<bool> r_stoped_;
  std::atomic<bool> s_started_;
  std::atomic<bool> r_started_;
  std::thread send_thread_;
  std::thread receive_thread_;
  PeerMessageCallback msg_cb_;
};

#endif  // SRC_PEER_PEER_H_
