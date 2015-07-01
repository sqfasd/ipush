#ifndef SRC_MESSAGE_H_
#define SRC_MESSAGE_H_

#include <ctype.h>
#include <string>
#include <ostream>
#include "deps/base/logging.h"
#include "src/typedef.h"
#include "src/utils.h"

namespace xcomet {

typedef shared_ptr<vector<string> > MessageDataSet;

const int64 NO_EXPIRE = 0;

#define K_FROM            'f'
#define K_TO              't'
#define K_SEQ             's'
#define K_TYPE            'y'
#define K_USER            'u'
#define K_CHANNEL         'c'
#define K_BODY            'b'
#define K_ROOM            'r'

// only used for peer conmunication, remove before save or send to user
#define K_TTL             'l'

struct MessagePrivate {
  int type;
  int seq;
  string to;
  string from;
  int64 ttl;
  string user;
  string channel;
  string body;
  string room;

  MessagePrivate() : type(-1), seq(-1), ttl(-1) {
  }
  bool operator==(const MessagePrivate& other) {
    return type == other.type &&
           seq == other.seq &&
           to == other.to &&
           from == other.from &&
           ttl == other.ttl &&
           user == other.user &&
           channel == other.channel &&
           body == other.body;
  }
};

class Message {
 public:
  enum MType {
    T_HEARTBEAT,  // Deprecated
    T_SUBSCRIBE,
    T_UNSUBSCRIBE,
    T_MESSAGE,
    T_CHANNEL_MESSAGE,
    T_ACK,
    T_ROOM_JOIN,
    T_ROOM_LEAVE,
    T_ROOM_KICK,
    T_ROOM_SEND,
    T_ROOM_BROADCAST,
    T_COUNT,
  };

  // type string only for stats use
  static const char* EnumTypeToString(MType type) {
    CHECK(type >= 0 && type < T_COUNT);
    static const char* const MTYPE_STRINGS[] = {
      "hb",  // Deprecated
      "sub",
      "unsub",
      "msg",
      "cmsg",
      "ack",
      "room_join",
      "room_leave",
      "room_kick",
      "room_msg",
      "room_broadcast",
    };
    return MTYPE_STRINGS[type];
  }

  Message() : p_(new MessagePrivate()) {}
  ~Message() {}
  bool operator==(const Message& other) {
    return *p_ == *(other.p_);
  }
  Message Clone() const {
    Message msg;
    *(msg.p_) = *(p_);
    return msg;
  }
  void SetFrom(const string& uid) {
    p_->from = uid;
  }
  void SetTo(const string& uid) {
    p_->to = uid;
  }
  void SetSeq(int seq) {
    p_->seq = seq;
  }
  void SetTTL(int64 ttl) {
    p_->ttl = ttl;
  }
  void SetType(MType type) {
    p_->type = (int)type;
  }
  void SetUser(const string& user) {
    p_->user = user;
  }
  void SetUser(const char* ptr) {
    p_->user.append(ptr);
  }
  void SetChannel(const string& channel) {
    p_->channel = channel;
  }
  void SetChannel(const char* ptr) {
    p_->channel.append(ptr);
  }
  void SetBody(const string& body) {
    p_->body = body;
  }
  void SetBody(const char* ptr, size_t len) {
    p_->body.append(ptr, len);
  }
  void SetBody(const char* ptr) {
    p_->body.append(ptr);
  }

  const string& From() const {
    return p_->from;
  }
  const string& To() const {
    return p_->to;
  }
  int Seq() const {
    return p_->seq;
  }
  int64 TTL() const {
    int64 ttl = p_->ttl;
    p_->ttl = -1;
    return ttl;
  }
  void RemoveTTL() {
    p_->ttl = -1;
  }
  MType Type() const {
    return (MType)p_->type;
  }
  const string& User() const {
    return p_->user;
  }
  const string& Channel() const {
    return p_->channel;
  }
  const string& Body() {
    return p_->body;
  }

  bool HasSeq() const {
    return p_->seq != -1;
  }
  bool HasType() const {
    return p_->type != -1;
  }
  bool HasTo() const {
    return !p_->to.empty();
  }

  void SetRoom(const string& room) {
    p_->room = room;
  }
  const string& Room() const {
    return p_->room;
  }

  static Message Unserialize(const StringPtr& data) {
    return UnserializeString(*data);
  }

  static bool SetField(char f, Message& m, const string& v) {
    VLOG(8) << f << ", " << v;
    switch (f) {
      case K_FROM:
        m.SetFrom(v);
        break;
      case K_TO:
        m.SetTo(v);
        break;
      case K_USER:
        m.SetUser(v);
        break;
      case K_CHANNEL:
        m.SetChannel(v);
        break;
      case K_BODY:
        m.SetBody(v);
        break;
      case K_TYPE:
        m.SetType(MType(std::stoi(v)));
        break;
      case K_SEQ:
        m.SetSeq(std::stoi(v));
        break;
      case K_TTL:
        m.SetTTL(std::stoll(v));
        break;
      default:
        LOG(ERROR) << "invalid message field: " << f;
        return false;
    }
    return true;
  }
  // need to bind, cannot overload
  static Message UnserializeString(const string& data) {
    enum ParseStatus {
      PS_NOT_START,
      PS_OBJ_START,
      PS_FIELD_START,
      PS_FIELD_FOUND,
      PS_FIELD_END,
      PS_VALUE_START,
      PS_VALUE_INT,
      PS_VALUE_STRING,
      PS_VALUE_STRING_ESCAPE,
      PS_VALUE_STRING_END,
      PS_OBJ_END,
    };
    Message msg;
    ParseStatus status = PS_NOT_START;
    ostringstream stream;
    char f = '\0';
    for (int i = 0; i < data.size(); ++i) {
      VLOG(8) << status << ", " << data[i];
      char c = data[i];
      if (c == ' ') {
        if (status == PS_VALUE_STRING) {
          stream << c; 
        }
        continue;
      }
      switch (status) {
        case PS_NOT_START:
          if (c == '{') {
            status = PS_OBJ_START;
          } else {
            goto __loop_end;
          }
          break;
        case PS_OBJ_START:
          if (c == '"') {
            status = PS_FIELD_START;
          } else {
            goto __loop_end;
          }
          break;
        case PS_FIELD_START:
          if (::isalpha(c)) {
            f = c;
            status = PS_FIELD_FOUND;
          } else {
            goto __loop_end;
          }
          break;
        case PS_FIELD_FOUND:
          if (c == '"') {
            status = PS_FIELD_END;
          } else {
            goto __loop_end;
          }
          break;
        case PS_FIELD_END:
          if (c == ':') {
            status = PS_VALUE_START;
            stream.str("");
          } else {
            goto __loop_end;
          }
          break;
        case PS_VALUE_START:
          if (c == '"') {
            status = PS_VALUE_STRING;
          } else if (::isdigit(c)) {
            status = PS_VALUE_INT;
            stream << c;
          }
          break;
        case PS_VALUE_STRING:
          if (c == '\\') {
            status = PS_VALUE_STRING_ESCAPE;
          } else if (c == '"') {
            status = PS_VALUE_STRING_END;
          } else {
            stream << c;
          }
          break;
        case PS_VALUE_STRING_ESCAPE:
          if (c == 'n') {
            stream << '\n';
          } else if (c == 'r') {
            stream << '\r';
          } else if (::isprint(c)) {
            stream << c;
          } else {
            goto __loop_end;
          }
          status = PS_VALUE_STRING;
          break;
        case PS_VALUE_STRING_END:
          if (c == ',') {
            SetField(f, msg, stream.str());
            status = PS_OBJ_START;
          } else if (c == '}') {
            SetField(f, msg, stream.str());
            status = PS_OBJ_END;
          } else {
            goto __loop_end;
          }
          break;
        case PS_VALUE_INT:
          if (c == ',') {
            SetField(f, msg, stream.str());
            status = PS_OBJ_START;
          } else if (c == '}') {
            SetField(f, msg, stream.str());
            status = PS_OBJ_END;
          } else {
            stream << c;
          }
          break;
        default:
          goto __loop_end;
      }
    }
    __loop_end:
    if (status != PS_OBJ_END) {
      LOG(ERROR) << "invalid message: " << data << "\n";
    }
    return msg;
  }

  static StringPtr Serialize(const Message& msg) {
    ostringstream stream;
    stream << "{";
    if (msg.p_->type != -1) {
      stream << '"' << K_TYPE << "\":" << msg.p_->type;
    }
    if (msg.p_->seq != -1) {
      stream << ",\"" << K_SEQ << "\":" << msg.p_->seq;
    }
    if (msg.p_->ttl != -1) {
      stream << ",\"" << K_TTL << "\":" << msg.p_->ttl;
    }
    if (!msg.p_->to.empty()) {
      stream << ",\"" << K_TO << "\":\"" << msg.p_->to << '"';
    }
    if (!msg.p_->from.empty()) {
      stream << ",\"" << K_FROM << "\":\"" << msg.p_->from << '"';
    }
    if (!msg.p_->user.empty()) {
      stream << ",\"" << K_USER << "\":\"" << msg.p_->user << '"';
    }
    if (!msg.p_->channel.empty()) {
      stream << ",\"" << K_CHANNEL << "\":\"" << msg.p_->channel << '"';
    }
    if (!msg.p_->room.empty()) {
      stream << ",\"" << K_ROOM << "\":\"" << msg.p_->room << '"';
    }
    if (!msg.p_->body.empty()) {
      stream << ",\"" << K_BODY << "\":\"";
      for (int i = 0; i < msg.p_->body.size(); ++i) {
        char c = msg.p_->body[i];
        if(c == '"') {
          stream << "\\\"";
        } else if (c == '\\') {
          stream << "\\\\";
        } else if(c == '\r') {
          stream << "\\r";
        } else if(c == '\n') {
          stream << "\\n";
        } else {
          stream << c;
        }
      }
      stream << '"';
    }
    stream << "}\n";
    StringPtr data(new string(stream.str()));
    return data;
  }

 private:
  shared_ptr<MessagePrivate> p_;

  friend ostream& operator<<(ostream&, const Message&);
};

inline ostream& operator<<(ostream& os, const Message& msg) {
  os << *(Message::Serialize(msg));
  return os;
}

}  // namespace xcomet
#endif  // SRC_MESSAGE_H_
