#include "gtest/gtest.h"

#include <atomic>
#include "deps/base/logging.h"
#include "src/include_std.h"
#include "src/peer/peer.h"

namespace xcomet {

Peer* CreatePeer(int peer_num, int id) {
  CHECK(peer_num > 1 && id < peer_num);
  vector<PeerInfo> peer_infos;
  for (int i = 0; i < peer_num; ++i) {
    if (id != i) {
      PeerInfo info;
      info.id = i;
      info.ip = "localhost";
      peer_infos.push_back(info);
    }
  }
  return new Peer(id, peer_infos);
}

const char* MSG_2_0 = "message from peer2 to peer0";
const char* MSG_0_1 = "message from peer0 to peer1";
const char* MSG_1_2 = "message from peer1 to peer2";
const char* MSG_BROADCAST = "broadcast by peer0";
static std::atomic<int> global_msg_count;
const int PEER_NUM = 3;
static Peer* peer0 = NULL;
static Peer* peer1 = NULL;
static Peer* peer2 = NULL;

void NormalTest() {
  CHECK(global_msg_count == 0);
  peer0->SetMessageCallback([](PeerMessagePtr msg) {
    LOG(INFO) << *msg;
    CHECK(msg->source == 2);
    CHECK(msg->target == 0);
    CHECK(msg->content == MSG_2_0);
    ++global_msg_count;
  });

  peer1->SetMessageCallback([](PeerMessagePtr msg) {
    LOG(INFO) << *msg;
    CHECK(msg->source == 0);
    CHECK(msg->target == 1);
    CHECK(msg->content == MSG_0_1);
    ++global_msg_count;
  });

  peer2->SetMessageCallback([](PeerMessagePtr msg) {
    LOG(INFO) << *msg;
    CHECK(msg->source == 1);
    CHECK(msg->target == 2);
    CHECK(msg->content == MSG_1_2);
    ++global_msg_count;
  });

  LOG(INFO) << "after start peers";
  ::sleep(2);
  LOG(INFO) << "before send to peers";

  peer0->Send(1, MSG_0_1);
  peer1->Send(2, MSG_1_2);
  peer2->Send(0, MSG_2_0);

  while (global_msg_count < 3) {
    LOG(INFO) << "waiting for msg callback";
    ::sleep(1);
  }
  LOG(INFO) << "reset message callback";

  peer1->SetMessageCallback([](PeerMessagePtr msg) {
    LOG(INFO) << *msg;
    CHECK(msg->source == 0);
    CHECK(msg->target == 1);
    CHECK(msg->content == MSG_BROADCAST);
    ++global_msg_count;
  });

  peer2->SetMessageCallback([](PeerMessagePtr msg) {
    LOG(INFO) << *msg;
    CHECK(msg->source == 0);
    CHECK(msg->target == 2);
    CHECK(msg->content == MSG_BROADCAST);
    ++global_msg_count;
  });

  LOG(INFO) << "broadcast ...";

  peer0->Broadcast(MSG_BROADCAST);

  while (global_msg_count < 5) {
    LOG(INFO) << "waiting for msg callback";
    ::sleep(1);
  }
  LOG(INFO) << "a moment after broadcast";
  CHECK(global_msg_count == 5) << global_msg_count;
}

TEST(PeerUnittest, Normal) {
  peer0 = CreatePeer(PEER_NUM, 0);
  peer1 = CreatePeer(PEER_NUM, 1);
  peer2 = CreatePeer(PEER_NUM, 2);

  global_msg_count = 0;
  peer0->Start();
  peer1->Start();
  peer2->Start();
  NormalTest();

  global_msg_count = 0;
  peer0->Restart();
  peer1->Restart();
  peer2->Restart();
  NormalTest();

  delete peer0;
  delete peer1;
  delete peer2;
}

}
