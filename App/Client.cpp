#include <iostream>
#include <map>


#include "Message.h"
#include "Nodes.h"


// Salticidae related stuff
#include "salticidae/msg.h"
#include "salticidae/event.h"
#include "salticidae/network.h"
#include "salticidae/stream.h"


using MsgNet = salticidae::MsgNetwork<uint8_t>;


const uint8_t Start::opcode;
const uint8_t Ping::opcode;
const uint8_t Reply::opcode;


salticidae::EventContext ec;
std::unique_ptr<MsgNet> net;          // network
std::map<PID,MsgNet::conn_t> conns;   // connections to the replicas
unsigned int numReplies = 0;
unsigned int numNodes = 1;
CID cid = 0;                          // id of this client


std::string cnfo() {
  return ("[C" + std::to_string(cid) + "]");
}


void send_start_to_all() {
  Start msg = Start(cid);
  for (auto &p: conns) { net->send_msg(msg, p.second); }
  std::cout << cnfo() << "start message sent to all" << KNRM << std::endl;
}


void handle_reply(Reply &&msg, const MsgNet::conn_t &conn) {
  numReplies++;
  std::cout << cnfo() << "received " << numReplies << " replies out of " << numNodes << KNRM << std::endl;
  if (numReplies == numNodes) {
    ec.stop();
  }
}


int main(int argc, char const *argv[]) {

  if (argc > 1) { sscanf(argv[1], "%d", &cid); }
  std::cout << "[C-id=" << cid << "]" << KNRM << std::endl;

  if (argc > 2) { sscanf(argv[2], "%d", &numNodes); }
  std::cout << cnfo() << "#nodes=" << numNodes << KNRM << std::endl;

  unsigned int size = std::max({sizeof(Start), sizeof(Reply)});
  MsgNet::Config config;
  config.max_msg_size(size);
  net = std::make_unique<MsgNet>(ec,config);

  net->start();

  Nodes nodes("config",numNodes);

  std::cout << cnfo() << "connecting..." << KNRM << std::endl;
  for (size_t j = 0; j < numNodes; j++) {
    NodeInfo * info = nodes.find(j);
    if (info != NULL) {

      std::cout << cnfo() << "connecting to: " << j << " " << info->host << " " << info->rport << KNRM << std::endl;
      salticidae::NetAddr peer_addr(info->host + ":" + std::to_string(info->cport));
      conns.insert(std::make_pair(j,net->connect_sync(peer_addr)));

    }
  }

  net->reg_handler(handle_reply);

  send_start_to_all();

  auto shutdown = [&](int) {ec.stop();};
  salticidae::SigEvent ev_sigterm(ec, shutdown);
  ev_sigterm.add(SIGTERM);

  std::cout << cnfo() << "dispatch" << KNRM << std::endl;
  ec.dispatch();

  return 0;
}
