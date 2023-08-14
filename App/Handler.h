#ifndef HANDLER_H
#define HANDLER_H


#include <map>

#include "Message.h"

// ------------------------------------
// SGX related stuff
#include "Enclave_u.h"
#include "sgx_urts.h"
#include "sgx_utils/sgx_utils.h"
// ------------------------------------

// ------------------------------------
// Salticidae related stuff
#include <memory>
#include <cstdio>
#include <functional>
#include "salticidae/msg.h"
#include "salticidae/event.h"
#include "salticidae/network.h"
#include "salticidae/stream.h"
// ------------------------------------

using std::placeholders::_1;
using std::placeholders::_2;


using PeerNet   = salticidae::PeerNetwork<uint8_t>;
using Peer      = std::tuple<PID,salticidae::PeerId>;
using Peers     = std::vector<Peer>;
using ClientNet = salticidae::ClientNetwork<uint8_t>;
using MsgNet    = salticidae::MsgNetwork<uint8_t>;
using ClientNfo = ClientNet::conn_t;
using Clients   = std::map<CID,ClientNfo>;
using rep_queue_t = salticidae::MPSCQueueEventDriven<std::pair<VAL,CID>>;


class Handler {

 private:
  PID myid;
  unsigned int numNodes = 1; // total number of nodes
  unsigned int numPings = 0;

  salticidae::EventContext pec; // peer ec
  salticidae::EventContext cec; // request ec
  Peers peers;
  PeerNet pnet;
  Clients clients;
  ClientNet cnet;
  std::thread c_thread; // request thread
  rep_queue_t rep_queue;
  salticidae::BoxObj<salticidae::ThreadCall> req_tcall;

  // used to print debugging info
  std::string nfo();

  // SGX functions
  COUNT callTEEadd();
  int initializeSGX();

  // Handlers
  void handle_ping(Ping msg, const PeerNet::conn_t &conn);
  void handle_start(Start msg, const ClientNet::conn_t &conn);


 public:
  Handler(PID id, unsigned int numNodes, PeerNet::Config pconf, ClientNet::Config cconf);
};


#endif
