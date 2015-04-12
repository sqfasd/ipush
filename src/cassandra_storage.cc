#include "cassandra_storage.h"

#include <algorithm>
#include "src/loop_executor.h"

//using namespace cass;

namespace xcomet {

#define MAX_BUFFER_LEN 512
#define ERROR_CASSANDRA "cassandra is error"

struct CassContext {
  CassSession* session;
};

template<typename CbType>
struct CbContext : public CassContext {
  CbType cb;
  string uid;
};

void RunCallback(const function<void()>& cb) {
  LoopExecutor::RunInMainLoop(cb);
}

template<typename CbType>
static CbContext<CbType>* CreateContext(const CbType& cb) {
  CbContext<CbType>* ctx = new CbContext<CbType>();
  ctx->cb = cb;
  return ctx;
}

static Error GetError(CassFuture* future) {
  const char* message = NULL;
  size_t message_length = 0;
  cass_future_error_message(future, &message, &message_length);
  if (message == NULL || message_length == 0) {
    return "NULL";
  }
  return message;
}

static void ExecuteQuery(CassStatement* statement,
                  CassFutureCallback callback,
                  CassContext* ctx) {
  CHECK(ctx != NULL);
  CHECK(ctx->session != NULL);
  CassFuture* future = cass_session_execute(ctx->session, statement);
  cass_future_set_callback(future, callback, ctx);
  cass_future_free(future);
  cass_statement_free(statement);
}

CassandraStorage::CassandraStorage() {
  cass_cluster_ = cass_cluster_new();
  CHECK(cass_cluster_);
  cass_cluster_set_contact_points(cass_cluster_, "192.168.2.3");

  cass_session_ = cass_session_new();
  CHECK(cass_session_);

  CassFuture* future = cass_session_connect_keyspace(
      cass_session_, cass_cluster_, "xcomet");
  cass_future_wait(future);
  CHECK(cass_future_error_code(future) == CASS_OK) << "cassandra connect failed";
  cass_future_free(future);
}

CassandraStorage::~CassandraStorage() {
  CassFuture* close_future = cass_session_close(cass_session_);
  cass_future_free(close_future);
  cass_cluster_free(cass_cluster_);
  cass_session_free(cass_session_);
}

static void OnGetMessage(CassFuture* future, void* data) {
  VLOG(5) << "OnGetMessage enter";
  auto ctx = static_cast<CbContext<GetMessageCallback>*>(data);
  if (cass_future_error_code(future) != CASS_OK) {
    RunCallback(bind(ctx->cb, GetError(future), MessageDataSet(NULL)));
    delete ctx;
    return;
  }

  MessageDataSet messages(new vector<string>());
  const CassResult* result = cass_future_get_result(future);
  CassIterator* iter = cass_iterator_from_result(result);
  while (cass_iterator_next(iter)) {
    const CassRow* row = cass_iterator_get_row(iter);
    const char* buf_ptr;
    size_t buf_len;
    cass_value_get_string(cass_row_get_column_by_name(row, "body"),
                          &buf_ptr,
                          &buf_len);
    messages->push_back(string());
    messages->back().reserve(buf_len);
    messages->back().assign(buf_ptr, buf_len);
  }
  VLOG(6) << "OnGetMessage size = " << messages->size();
  if (FLAGS_v >= 6) {
    for (int i = 0; i < messages->size(); ++i) {
      VLOG(6) << i << ": " << messages->at(i);
    }
  }
  std::reverse(messages->begin(), messages->end());
  cass_iterator_free(iter);
  cass_result_free(result);
  RunCallback(bind(ctx->cb, NO_ERROR, messages));
  delete ctx;
}

static void OnGetLastAck(CassFuture* future, void* data) {
  VLOG(5) << "OnGetLastAck enter";
  auto ctx = static_cast<CbContext<GetMessageCallback>*>(data);
  if (cass_future_error_code(future) != CASS_OK) {
    RunCallback(bind(ctx->cb, GetError(future), MessageDataSet(NULL)));
    delete ctx;
    return;
  }
  int last_ack = 0;
  const CassResult* result = cass_future_get_result(future);
  CassIterator* iter = cass_iterator_from_result(result);
  if (cass_iterator_next(iter)) {
    const CassRow* row = cass_iterator_get_row(iter);
    cass_value_get_int32(cass_row_get_column_by_name(row, "last_ack"),
                         &last_ack);
    cass_iterator_free(iter);
    cass_result_free(result);
  }
  VLOG(6) << "OnGetLastAck last_ack = " << last_ack;
  const char* query = "SELECT body FROM message WHERE uid = ? AND seq > ?"
                      " order by seq DESC limit 100;";
  CassStatement* statement = cass_statement_new(query, 2);
  cass_statement_bind_string(statement, 0, ctx->uid.c_str());
  cass_statement_bind_int32(statement, 1, last_ack);
  ExecuteQuery(statement, OnGetMessage, ctx);
}

void CassandraStorage::GetMessage(const string& uid,
                                  GetMessageCallback callback) {
  VLOG(5) << "GetMessage enter";
  auto ctx = CreateContext(callback);
  ctx->cb = callback;
  ctx->uid = uid;
  ctx->session = cass_session_;
  const char* query = "SELECT last_ack FROM user where uid = ?;";
  CassStatement* statement = cass_statement_new(query, 1);
  cass_statement_bind_string(statement, 0, uid.c_str());
  ExecuteQuery(statement, OnGetLastAck, ctx);
}

static void OnSaveMessage(CassFuture* future, void* data) {
  VLOG(5) << "OnSaveMessage enter";
  auto ctx = static_cast<CbContext<SaveMessageCallback>*>(data);
  if (cass_future_error_code(future) != CASS_OK) {
    RunCallback(bind(ctx->cb, GetError(future)));
  } else {
    RunCallback(bind(ctx->cb, NO_ERROR));
  }
  delete ctx;
}

void CassandraStorage::SaveMessage(const StringPtr& msg,
                                   const string& uid,
                                   int seq,
                                   int64 ttl,
                                   SaveMessageCallback callback) {
  VLOG(5) << "SaveMessage enter";
  auto ctx = CreateContext(callback);
  ctx->session = cass_session_;
  const char* query = "INSERT INTO message (uid, seq, body)"
                      " VALUES (?, ?, ?) using ttl ?;";
  CassStatement* statement = cass_statement_new(query, 4);
  cass_statement_bind_string(statement, 0, uid.c_str());
  cass_statement_bind_int32(statement, 1, seq);
  cass_statement_bind_string(statement, 2, msg->c_str());
  cass_statement_bind_int32(statement, 3, ttl);
  ExecuteQuery(statement, OnSaveMessage, ctx);
}

static void OnUpdateAck(CassFuture* future, void* data) {
  VLOG(5) << "OnUpdateAck enter";
  auto ctx = static_cast<CbContext<UpdateAckCallback>*>(data);
  if (cass_future_error_code(future) != CASS_OK) {
    RunCallback(bind(ctx->cb, GetError(future)));
  } else {
    RunCallback(bind(ctx->cb, NO_ERROR));
  }
  delete ctx;
}

void CassandraStorage::UpdateAck(const string& uid,
                                 int ack_seq,
                                 UpdateAckCallback callback) {
  VLOG(5) << "UpdateAck enter";
  auto ctx = CreateContext(callback);
  ctx->session = cass_session_;
  const char* query = "UPDATE user SET last_ack = ? WHERE uid = ?;";
  CassStatement* statement = cass_statement_new(query, 2);
  cass_statement_bind_int32(statement, 0, ack_seq);
  cass_statement_bind_string(statement, 1, uid.c_str());
  ExecuteQuery(statement, OnUpdateAck, ctx);
}

static void OnGetMaxSeq(CassFuture* future, void* data) {
  VLOG(5) << "OnGetMaxSeq enter";
  auto ctx = static_cast<CbContext<GetMaxSeqCallback>*>(data);
  if (cass_future_error_code(future) != CASS_OK) {
    RunCallback(bind(ctx->cb, GetError(future), -1));
    delete ctx;
    return;
  }
  int max_seq = 0;
  const CassResult* result = cass_future_get_result(future);
  CassIterator* iter = cass_iterator_from_result(result);
  if (cass_iterator_next(iter)) {
    const CassRow* row = cass_iterator_get_row(iter);
    cass_value_get_int32(cass_row_get_column_by_name(row, "seq"),
                         &max_seq);
  }
  VLOG(6) << "OnGetMaxSeq: " << max_seq;
  cass_result_free(result);
  cass_iterator_free(iter);
  RunCallback(bind(ctx->cb, NO_ERROR, max_seq));
  delete ctx;
}

void CassandraStorage::GetMaxSeq(const string& uid,
                                 GetMaxSeqCallback callback) {
  VLOG(5) << "GetMaxSeq enter";
  auto ctx = CreateContext(callback);
  ctx->session = cass_session_;
  const char* query = "SELECT seq FROM message WHERE uid = ?"
                      " ORDER BY seq DESC limit 1;";
  CassStatement* statement = cass_statement_new(query, 1);
  cass_statement_bind_string(statement, 0, uid.c_str());
  ExecuteQuery(statement, OnGetMaxSeq, ctx);
}

static void OnAddUserToChannel(CassFuture* future, void* data) {
  VLOG(5) << "OnAddUserToChannel enter";
  auto ctx = static_cast<CbContext<AddUserToChannelCallback>*>(data);
  if (cass_future_error_code(future) != CASS_OK) {
    RunCallback(bind(ctx->cb, GetError(future)));
  } else {
    RunCallback(bind(ctx->cb, NO_ERROR));
  }
  delete ctx;
}

void CassandraStorage::AddUserToChannel(const string& uid,
                                        const string& cid,
                                        AddUserToChannelCallback callback) {
  VLOG(5) << "AddUserToChannel enter";
  auto ctx = CreateContext(callback);
  ctx->session = cass_session_;
  const char* query = "INSERT INTO channel (cid, uid) VALUES (?, ?);";
  CassStatement* statement = cass_statement_new("", 2);
  cass_statement_bind_string(statement, 0, cid.c_str());
  cass_statement_bind_string(statement, 1, uid.c_str());
  ExecuteQuery(statement, OnAddUserToChannel, ctx);
}

static void OnRemoveUserFromChannel(CassFuture* future, void* data) {
  VLOG(5) << "OnRemoveUserFromChannel enter";
  auto ctx = static_cast<CbContext<RemoveUserFromChannelCallback>*>(data);
  if (cass_future_error_code(future) != CASS_OK) {
    RunCallback(bind(ctx->cb, GetError(future)));
  } else {
    RunCallback(bind(ctx->cb, NO_ERROR));
  }
  delete ctx;
}

void CassandraStorage::RemoveUserFromChannel(
    const string& uid,
    const string& cid,
    RemoveUserFromChannelCallback callback) {
  VLOG(5) << "RemoveUserFromChannel enter";
  auto ctx = CreateContext(callback);
  ctx->session = cass_session_;
  const char* query = "DELETE FROM channel WHERE cid=? AND uid=?;";
  CassStatement* statement = cass_statement_new(query, 2);
  cass_statement_bind_string(statement, 0, cid.c_str());
  cass_statement_bind_string(statement, 1, uid.c_str());
  ExecuteQuery(statement, OnRemoveUserFromChannel, ctx);
}

static void OnGetChannelUsers(CassFuture* future, void* data) {
  VLOG(5) << "OnGetChannelUsers enter";
  auto ctx = static_cast<CbContext<GetChannelUsersCallback>*>(data);
  if (cass_future_error_code(future) != CASS_OK) {
    RunCallback(bind(ctx->cb, GetError(future), UserResultSet(NULL)));
    delete ctx;
    return;
  }
  UserResultSet users(new vector<string>());
  const CassResult* result = cass_future_get_result(future);
  CassIterator* iter = cass_iterator_from_result(result);
  while (cass_iterator_next(iter)) {
    const CassRow* row = cass_iterator_get_row(iter);
    const char* buf_ptr;
    size_t buf_len;
    cass_value_get_string(cass_row_get_column_by_name(row, "uid"),
                          &buf_ptr,
                          &buf_len);
    users->push_back(string());
    users->back().reserve(buf_len);
    users->back().assign(buf_ptr, buf_len);
  }
  VLOG(6) << "OnGetChannelUsers size = " << users->size();
  if (FLAGS_v >= 6) {
    for (int i = 0; i < users->size(); ++i) {
      VLOG(6) << i << ": " << users->at(i);
    }
  }
  cass_result_free(result);
  cass_iterator_free(iter);
  RunCallback(bind(ctx->cb, NO_ERROR, users));
  delete ctx;
}

void CassandraStorage::GetChannelUsers(const string& cid,
                                       GetChannelUsersCallback callback) {
  VLOG(5) << "GetChannelUsers enter";
  auto ctx = CreateContext(callback);
  ctx->session = cass_session_;
  const char* query = "SELECT uid FROM channel WHERE cid=?;";
  CassStatement* statement = cass_statement_new(query, 1);
  cass_statement_bind_string(statement, 0, cid.c_str());
  ExecuteQuery(statement, OnGetChannelUsers, ctx);
}

}  // namespace xcomet
