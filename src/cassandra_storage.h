#ifndef SRC_CASSANDRASTORAGE_H_
#define SRC_CASSANDRASTORAGE_H_

#include "deps/cassandra/cpp-driver/include/cassandra.h"
#include "src/storage.h"

namespace xcomet {

class CassandraStorage : public Storage {
 public:
  CassandraStorage();
  ~CassandraStorage();
 private:
  virtual void GetMessage(const string& uid, GetMessageCallback callback);
  virtual void SaveMessage(const StringPtr& msg,
                           const string& uid,
                           int seq,
                           int64 ttl,
                           SaveMessageCallback callback);
  virtual void UpdateAck(const string& uid,
                         int ack_seq,
                         UpdateAckCallback callback);

  virtual void GetMaxSeq(const string& uid, GetMaxSeqCallback callback);


  virtual void AddUserToChannel(const string& uid,
                                const string& cid,
                                AddUserToChannelCallback callback);
  virtual void RemoveUserFromChannel(const string& uid,
                                     const string& cid,
                                     RemoveUserFromChannelCallback callback);
  virtual void GetChannelUsers(const string& cid,
                               GetChannelUsersCallback callback);

  CassSession* cass_session_;
  CassCluster* cass_cluster_;
};

}
#endif  // SRC_CASSANDRASTORAGE_H_
