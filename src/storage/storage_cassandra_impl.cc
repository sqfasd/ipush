

int GetOfflineMessages(const string user, vector<MessagePtr> messages);
int SaveMessage(const string user, MessagePtr message);
int UpdateLastAck(const string user, int ack);
int GetMessageSeq(const string user);

int AddUserToGroup(const string user, const string group);
int RemoveUserFromGroup(constt string user, const string group);
int GetGroupUsers(const string user);

