#include "gtest/gtest.h"

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

TEST(PeerUnittest, Normal) {
  const int PEER_NUM = 3;
  Peer* peer0 = CreatePeer(PEER_NUM, 0);
  peer0->SetMessageCallback([](PeerMessagePtr msg) {
    CHECK(msg->source == 2);
    CHECK(msg->target == 0);
    CHECK(msg->content == MSG_2_0);
  });

  Peer* peer1 = CreatePeer(PEER_NUM, 1);
  peer1->SetMessageCallback([](PeerMessagePtr msg) {
    CHECK(msg->source == 0);
    CHECK(msg->target == 1);
    CHECK(msg->content == MSG_0_1);
  });

  Peer* peer2 = CreatePeer(PEER_NUM, 2);
  peer2->SetMessageCallback([](PeerMessagePtr msg) {
    CHECK(msg->source == 1);
    CHECK(msg->target == 2);
    CHECK(msg->content == MSG_1_2);
  });

  peer0->Start();
  peer1->Start();
  peer2->Start();

  ::sleep(1);

  peer0->Send(1, MSG_0_1);
  peer1->Send(2, MSG_1_2);
  peer2->Send(0, MSG_2_0);

  ::sleep(1);

  delete peer0;
  delete peer1;
  delete peer2;
}

}
