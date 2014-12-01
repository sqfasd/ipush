#include "deps/base/string_util.h"
#include "src/router_proxy.h"
#include "src/http_query.h"

namespace xcomet {

RouterProxy::RouterProxy()
    : counter_(0), buffer_size_(1000), seq_(0), offset_(0), tail_(0) {
  msg_buffer_.reserve(buffer_size_);
}

RouterProxy::~RouterProxy() {
}

void RouterProxy::OnSessionDisconnected() {
  LOG(WARNING) << "RouterProxy disconnected";
}

void RouterProxy::ResetSession(struct evhttp_request* req) {
  session_.reset(new Session(req));
  session_->SetDisconnectCallback(
      base::NewOneTimeCallback(this, &RouterProxy::OnSessionDisconnected));
  HttpQuery query(req);
  offset_ = query.GetInt("seq", 0);
  SendAll();
}

void RouterProxy::SendAll() {
  if (session_.get()) {
    while (offset_ < tail_) {
      session_->Send2(msg_buffer_[offset_]);
      offset_++;
    }
  }
}

void RouterProxy::Redirect(const string& uid, const string& content) {
  VLOG(3) << "Redirect: uid=" << uid << ", content=" << content;
  string msg = StringPrintf("{\"type\":\"user_msg\", \"content\":\"%s\",\"uid\":\"%s\",\"seq\":%d}",
      uid.c_str(), content.c_str(), seq_++);
  msg_buffer_.push_back(msg);
  tail_++;
  SendAll();
}

void RouterProxy::Redirect(struct evhttp_request* pub_req) {
  VLOG(3) << "Redirect: " << evhttp_request_get_uri(pub_req);
  HttpQuery query(pub_req);
  string content = query.GetStr("content", "");
  string uid = query.GetStr("uid", "");
  if (uid.empty()) {
    return;
  }
  if (users_.find(uid) != users_.end()) {
    LOG(INFO) << "[FIXME] Redirect: uid="
      << uid << ", content=" << content;
  } else {
    LOG(INFO) << "[FIXME] save offline msg: uid=" << uid
      << ", content" << content;
    user_msgs_[uid].push(content);
  }
}

void RouterProxy::RegisterUser(const string& uid) {
  LOG(INFO) << "RegisterUser: " << uid;
  // users_.insert(uid);
  string msg = StringPrintf("{\"type\":\"login\", \"uid\":\"%s\",\"seq\":%d}",
      uid.c_str(), seq_++);
  // msg_buffer_.push_back(msg);
  // tail_++;
  // SendAll();
  if (session_.get()) {
    session_->Send2(msg);
  } else {
    LOG(ERROR) << "session is not available";
  }
}

void RouterProxy::UnregisterUser(const string& uid) {
  // users_.erase(uid);
  string msg = StringPrintf("{\"type\":\"logout\", \"uid\":\"%s\",\"seq\":%d}",
      uid.c_str(), seq_++);
  // msg_buffer_.push_back(msg);
  // tail_++;
  // SendAll();
  if (session_.get()) {
    session_->Send2(msg);
  } else {
    LOG(ERROR) << "session is not available";
  }
}

void RouterProxy::ChannelBroadcast(const string& cid, const string& content) {
  LOG(INFO) << "[FIXME] ChannelBroadcast: cid="
    << cid << ", content=" << content;
}

queue<string>* RouterProxy::GetOfflieMessages(const string& uid) {
  return &user_msgs_[uid];
}

bool RouterProxy::HasOfflineMessage(const string& uid) {
  MsgMap::iterator iter = user_msgs_.find(uid);
  if (iter != user_msgs_.end()) {
    return !iter->second.empty();
  }
  return false;
}

void RouterProxy::RemoveOfflineMessages(const string& uid) {
  user_msgs_.erase(uid);
}

void RouterProxy::SendHeartbeat() {
  if (session_.get()) {
    session_->SendHeartbeat();
  }
}

}  // namespace xcomet
