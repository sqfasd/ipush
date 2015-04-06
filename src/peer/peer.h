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
};

struct PeerMessage {
  int source;
  int target;
  string content;
};

typedef shared_ptr<PeerMessage> PeerMessagePtr;
typedef function<void (PeerMessagePtr)> PeerMessageCallback;

class Peer {
 public:
  Peer(const int id, const vector<PeerInfo>& peers);
  ~Peer();
  void Start();
  void Stop();
  void Broadcast(string& content);
  void Broadcast(const char* content);
  void Send(const int target, string& content);
  void Send(const int target, const char* content);
  void SetMessageCallback(const PeerMessageCallback& cb) {msg_cb_ = cb;}

 private:
  void Sending();
  void Receiving();

  const int id_;
  vector<PeerInfo> peers_;
  base::ConcurrentQueue<PeerMessagePtr> outbox_;
  std::atomic<bool> stoped_;
  std::thread send_thread_;
  std::thread recv_thread_;
  PeerMessageCallback msg_cb_;
};

#endif  // SRC_PEER_PEER_H_
