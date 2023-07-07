#ifndef MSG_H
#define MSG_H

#include <set>


#include "salticidae/msg.h"
#include "salticidae/stream.h"


#include "config.h"


struct Start {
  static const uint8_t opcode = HDR_START;
  salticidae::DataStream serialized;
  CID cid;
  Start(const int &cid) : cid(cid) { serialized << cid; }
  Start(salticidae::DataStream &&s) { s >> cid; }
  bool operator<(const Start& s) const {
    return (cid < s.cid);
  }
  std::string prettyPrint() {
    return "START[" + std::to_string(cid) + "]";
  }
  unsigned int sizeMsg() { return (sizeof(CID)); }
  void serialize(salticidae::DataStream &s) const { s << cid; }
};


struct Ping {
  static const uint8_t opcode = HDR_PING;
  salticidae::DataStream serialized;
  VAL val;
  CID cid;
  Ping(const VAL &val, const CID &cid) : val(val), cid(cid) { serialized << val << cid; }
  Ping(salticidae::DataStream &&s) { s >> val >> cid; }
  bool operator<(const Ping& s) const {
    return (val < s.val);
  }
  std::string prettyPrint() {
    return "PING[" + std::to_string(val) + "," + std::to_string(cid) + "]";
  }
  unsigned int sizeMsg() { return (sizeof(VAL) + sizeof(CID)); }
  void serialize(salticidae::DataStream &s) const { s << val << cid; }
};


struct Reply {
  static const uint8_t opcode = HDR_REPLY;
  salticidae::DataStream serialized;
  VAL val;
  Reply(const int &val) : val(val) { serialized << val; }
  Reply(salticidae::DataStream &&s) { s >> val; }
  bool operator<(const Reply& s) const {
    return (val < s.val);
  }
  std::string prettyPrint() {
    return "REPLY[" + std::to_string(val) + "]";
  }
  unsigned int sizeMsg() { return (sizeof(VAL)); }
  void serialize(salticidae::DataStream &s) const { s << val; }
};

#endif
