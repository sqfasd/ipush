#include "src/peer/peer.h"

#include <utility>
#include "deps/base/logging.h"
#include "src/peer/zhelpers.h"

const int IO_THREAD_NUM = 1;
const int ALL_PEERS = -1;
DEFINE_int32(peer_start_port, 11000, "");

Peer::Peer(const int id, const vector<PeerInfo>& peers)
    : id_(id),
      s_stoped_(false),
      r_stoped_(false),
      s_started_(false),
      r_started_(false) {
  for (int i = 0; i < peers.size(); ++i) {
    if (peers[i].id != id_) {
      peers_.push_back(peers[i]);
    }
  }
}

Peer::~Peer() {
  Stop();
  VLOG(3) << "Peer::~Peer " << id_;
}

void Peer::Start() {
  if (peers_.size() == 0) {
    LOG(INFO) << "single mode, will not start peer service";
    s_stoped_ = true;
    r_stoped_ = true;
    return;
  }
  StartSend();
  StartReceive();
}

void Peer::StartSend() {
  if (s_started_ || send_thread_.joinable()) {
    LOG(WARNING) << "sending thread already started";
    return;
  }
  s_stoped_ = false;
  s_started_ = false;
  send_thread_ = std::thread(&Peer::Sending, this);
  while (!s_started_) {
    LOG(INFO) << "waiting for sending thread start";
    ::sleep(1);
  }
}

void Peer::StartReceive() {
  if (r_started_ || receive_thread_.joinable()) {
    LOG(WARNING) << "receiving thread already started";
    return;
  }
  r_stoped_ = false;
  r_started_ = false;
  receive_thread_ = std::thread(&Peer::Receiving, this);
  while (!r_started_) {
    LOG(INFO) << "waiting for receiving thread start";
    ::sleep(1);
  }
}

void Peer::Broadcast(string& content) {
  Send(ALL_PEERS, content);
}

void Peer::Broadcast(const char* content) {
  Send(ALL_PEERS, content);
}

void Peer::Send(const int target, string& content) {
  PeerMessagePtr msg(new PeerMessage());
  msg->target = target;
  msg->content.swap(content);
  outbox_.Push(msg);
}

void Peer::Send(const int target, const char* content) {
  string str(content);
  Send(target, str);
}

void Peer::StopSend() {
  if (!s_stoped_) {
    s_stoped_ = true;
    outbox_.Push(shared_ptr<PeerMessage>(NULL));
  }
  if (send_thread_.joinable()) {
    LOG(INFO) << "join send thread";
    send_thread_.join();
  }
  LOG(INFO) << "send thread stoped";
}

void Peer::StopReceive() {
  if (!r_stoped_) {
    r_stoped_ = true;
  }
  if (receive_thread_.joinable()) {
    LOG(INFO) << "join receive thread";
    receive_thread_.join();
  }
  LOG(INFO) << "receive thread stoped";
}

void Peer::Stop() {
  VLOG(3) << "Peer::Stop";
  StopSend();
  StopReceive();
  LOG(INFO) << "peer stoped";
}

void Peer::Restart() {
  Stop();
  Start();
}

void Peer::RestartReceive() {
  LOG(INFO) << "RestartReceive ...";
  thread th([this]() {
    StopReceive();
    StartReceive();
  });
  LOG(INFO) << "wait for RestartReceive finish";
  th.join();
}

void Peer::RestartSend() {
  LOG(INFO) << "RestartSend ...";
  thread th([this]() {
    StopSend();
    StartSend();
  });
  LOG(INFO) << "wait for RestartSend finish";
  th.join();
}

void Peer::Sending() {
  zmq::context_t context(IO_THREAD_NUM);
  zmq::socket_t publisher(context, ZMQ_PUB);
  string address = "tcp://*:" + std::to_string(FLAGS_peer_start_port + id_);
  publisher.bind(address.c_str());
  
  LOG(INFO) << "ready to publish: " << id_;
  s_started_ = true;

  while (!s_stoped_) {
    try {
      //  Write two messages, each with an envelope and content
      PeerMessagePtr msg;
      outbox_.Pop(msg);
      if (msg.get()) {
        if (msg->target == ALL_PEERS) {
          for (int i = 0; i < peers_.size(); ++i) {
            s_sendmore (publisher, std::to_string(peers_[i].id));
            s_send (publisher, msg->content);
          }
        } else {
          s_sendmore (publisher, std::to_string(msg->target));
          s_send (publisher, msg->content);
        }
      }
    } catch (std::exception& e) {
     LOG(WARNING) << "zmq sending exception: " << e.what();
    }
  }
  s_stoped_ = true;
  s_started_ = false;
  LOG(INFO) << "sending loop exited";
}

void Peer::Receiving() {
  zmq::pollitem_t* poll_items = new zmq::pollitem_t[peers_.size()];
  zmq::context_t context(IO_THREAD_NUM);
  vector<shared_ptr<zmq::socket_t> > sockets(peers_.size());
  for (int i = 0; i < peers_.size(); ++i) {
    sockets[i].reset(new zmq::socket_t(context, ZMQ_SUB));
    string address = "tcp://" + peers_[i].ip + ":" +
                     std::to_string(FLAGS_peer_start_port + peers_[i].id);
    LOG(INFO) << "connecting to peer: " << address;
    sockets[i]->connect(address.c_str());
    sockets[i]->setsockopt(ZMQ_SUBSCRIBE, std::to_string(id_).c_str(), 1);
    poll_items[i] = {*sockets[i], 0, ZMQ_POLLIN, 0};
  }

  LOG(INFO) << "ready to receive: " << id_;
  r_started_ = true;

  while (!r_stoped_) {
    try {
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
    } catch (std::exception& e) {
      LOG(WARNING) << "zmq receiving exception: " << e.what();
    }
  }
  r_stoped_ = true;
  r_started_ = false;
  delete [] poll_items;
  LOG(INFO) << "receiving loop exited";
}
