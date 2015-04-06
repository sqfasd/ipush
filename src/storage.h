#ifndef SRC_STORAGE_H_
#define SRC_STORAGE_H_

#include <inttypes.h>
#include "src/include_std.h"
#include "src/message.h"

namespace xcomet {

typedef shared_ptr<vector<string> > UserResultSet;
typedef shared_ptr<string> ErrorPtr;

#define NO_ERROR ErrorPtr(NULL)

typedef function<void (ErrorPtr, MessageDataSet)> GetMessageCallback;
typedef function<void (ErrorPtr)> SaveMessageCallback;
typedef function<void (ErrorPtr)> UpdateAckCallback;
typedef function<void (ErrorPtr, int)> GetMaxSeqCallback;
typedef function<void (ErrorPtr)> AddUserToChannelCallback;
typedef function<void (ErrorPtr)> RemoveUserFromChannelCallback;
typedef function<void (ErrorPtr, UserResultSet)> GetChannelUsersCallback;

class Storage {
 public:
  Storage() {}
  virtual ~Storage() {}

  virtual void SaveMessage(MessagePtr msg,
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
