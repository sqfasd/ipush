#ifndef SRC_STORAGE_H_
#define SRC_STORAGE_H_

#include <inttypes.h>
#include "src/include_std.h"
#include "src/message.h"
#include "src/typedef.h"

namespace xcomet {

typedef shared_ptr<vector<string> > UserResultSet;

typedef function<void (Error, MessageDataSet)> GetMessageCallback;
typedef function<void (Error)> SaveMessageCallback;
typedef function<void (Error)> UpdateAckCallback;
typedef function<void (Error, int)> GetMaxSeqCallback;
typedef function<void (Error)> AddUserToChannelCallback;
typedef function<void (Error)> RemoveUserFromChannelCallback;
typedef function<void (Error, UserResultSet)> GetChannelUsersCallback;

class Storage {
 public:
  Storage() {}
  virtual ~Storage() {}

  virtual void SaveMessage(const StringPtr& msg,
                           const string& uid,
                           int64 ttl,
                           SaveMessageCallback cb) = 0;
  virtual void GetMessage(const string& uid, GetMessageCallback cb) = 0;
  virtual void GetMaxSeq(const string& uid, GetMaxSeqCallback cb) = 0;
  virtual void UpdateAck(const string& uid,
                         int ack_seq,
                         UpdateAckCallback cb) = 0;

  virtual void AddUserToChannel(const string& uid,
                                const string& cid,
                                AddUserToChannelCallback cb) = 0;
  virtual void RemoveUserFromChannel(const string& uid,
                                     const string& cid,
                                     RemoveUserFromChannelCallback cb) = 0;
  virtual void GetChannelUsers(const string& cid,
                               GetChannelUsersCallback cb) = 0;
};
}  // namespace xcomet
#endif  // SRC_STORAGE_H_
