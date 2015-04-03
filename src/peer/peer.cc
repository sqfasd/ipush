#include "src/peer/peer.h"

#include "deps/base/logging.h"
#include "src/peer/zhelpers.h"

const int IO_THREAD_NUM = 1;
const int DEFAULT_START_PORT = 11000;

Peer::Peer(const int id, const vector<PeerInfo>& peers)
    : id_(id), peers_(peers), stoped_(false) {
}

Peer::~Peer() {
  VLOG(3) << "Peer::~Peer";
  if (!stoped_) {
    Stop();
  }
}

void Peer::Start() {
  send_thread_ = std::thread(&Peer::Sending, this);
  recv_thread_ = std::thread(&Peer::Receiving, this);
}

void Peer::Send(const int target, const string& content) {
  PeerMessagePtr msg(new PeerMessage());
  msg->target = target;
  msg->content = content;
  outbox_.Push(msg);
}

void Peer::Stop() {
  VLOG(3) << "Peer::Stop";
  stoped_ = true;
  outbox_.Push(shared_ptr<PeerMessage>(NULL));
  if (send_thread_.joinable()) {
    send_thread_.join();
  }
  if (recv_thread_.joinable()) {
    recv_thread_.join();
  }
  LOG(INFO) << "peer stoped";
}

void Peer::Sending() {
  zmq::context_t context(IO_THREAD_NUM);
  zmq::socket_t publisher(context, ZMQ_PUB);
  string address = "tcp://*:" + std::to_string(DEFAULT_START_PORT + id_);
  publisher.bind(address.c_str());
  
  LOG(INFO) << "ready to publish: " << address;

  while (!stoped_) {
    //  Write two messages, each with an envelope and content
    PeerMessagePtr msg;
    outbox_.Pop(msg);
    if (msg.get()) {
      s_sendmore (publisher, std::to_string(msg->target));
      s_send (publisher, msg->content);
    }
  }
  LOG(INFO) << "sending loop exited";
}

void Peer::Receiving() {
  zmq::context_t context(IO_THREAD_NUM);
  vector<shared_ptr<zmq::socket_t> > sockets(peers_.size());
  zmq::pollitem_t* poll_items = new zmq::pollitem_t[peers_.size()];
  for (int i = 0; i < peers_.size(); ++i) {
    sockets[i].reset(new zmq::socket_t(context, ZMQ_SUB));
    string address = "tcp://" + peers_[i].ip + ":" +
                     std::to_string(DEFAULT_START_PORT + peers_[i].id);
    LOG(INFO) << "connecting to peer: " << address;
    sockets[i]->connect(address.c_str());
    sockets[i]->setsockopt(ZMQ_SUBSCRIBE, std::to_string(id_).c_str(), 1);
    poll_items[i] = {*sockets[i], 0, ZMQ_POLLIN, 0};
  }

  while (!stoped_) {
    zmq::message_t message;
    const int DEFAULT_TIMEOUT_MS = 100;
    zmq::poll(&poll_items[0], peers_.size(), DEFAULT_TIMEOUT_MS);

    for (int i = 0; i < peers_.size(); ++i) {
      if (poll_items[i].revents & ZMQ_POLLIN) {
        PeerMessagePtr pmsg(new PeerMessage());
        pmsg->target = std::stoi(s_recv(*sockets[i]));
        pmsg->content = s_recv(*sockets[i]);
        pmsg->source = peers_[i].id;
        VLOG(4) << "from peer [" << peers_[i].id << "] " << pmsg->content;
        if (msg_cb_) {
          msg_cb_(pmsg);
        }
      }
    }
  }
  delete [] poll_items;
  LOG(INFO) << "receiving loop exited";
}
