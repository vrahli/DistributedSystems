#include <iostream>
#include <cstring>
#include <fstream>


#include "config.h"
#include "Nodes.h"
#include "Handler.h"


void setPayload(std::string s, payload_t *p) {
  p->size=s.size();
  memcpy(p->data,s.c_str(),s.size()); // MAX_SIZE_PAYLOAD
}

// ------------------------------------
// SGX related stuff
/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

int Handler::initializeSGX() {
  // Initializing enclave
  if (initialize_enclave(&global_eid, "enclave.token", "enclave.signed.so") < 0) {
    std::cout << nfo() << "Failed to initialize enclave" << std::endl;
    return 1;
  }
  if (DEBUG) std::cout << KBLU << nfo() << "initialized enclave" << KNRM << std::endl;

  return 0;
}

// OCalls
void ocall_print(const char* str) { printf("%s\n", str); }

// ECalls
COUNT Handler::callTEEadd() {
  COUNT ci = 1;
  COUNT co = 0;
  sgx_status_t ret;
  sgx_status_t status = TEEadd(global_eid, &ret, &ci, &co);
  return co;
}

auth_t Handler::callTEEsign(std::string s) {
  payload_t p;
  setPayload(s, &p);
  auth_t a;
  sgx_status_t ret;
  sgx_status_t status = TEEsign(global_eid, &ret, &p, &a);
  return a;
}

bool Handler::callTEEverify(std::string s, auth_t a) {
  payload_t p;
  setPayload(s, &p);
  bool b = false;
  sgx_status_t ret;
  sgx_status_t status = TEEverify(global_eid, &ret, &p, &a, &b);
  return b;
}
// ------------------------------------


std::string Handler::nfo() { return "[" + std::to_string(this->myid) + "]"; }

const uint8_t Start::opcode;
const uint8_t Ping::opcode;
const uint8_t Reply::opcode;

Handler::Handler(PID id, unsigned int numNodes, PeerNet::Config pconf, ClientNet::Config cconf) :
pnet(pec,pconf), cnet(cec,cconf) {
  this->myid     = id;
  this->numNodes = numNodes;

  // -- SGX
  initializeSGX();

  // -- Salticidae
  req_tcall = new salticidae::ThreadCall(cec);
  // the client event context handles replies through the 'rep_queue' queue
  rep_queue.reg_handler(cec, [this](rep_queue_t &q) {
                               std::pair<VAL,CID> p;
                               while (q.try_dequeue(p)) {
                                 VAL val = p.first;
                                 CID cid = p.second;
                                 Clients::iterator cit = this->clients.find(cid);
                                 if (cit != this->clients.end()) {
                                   ClientNet::conn_t recipient = cit->second;
                                   Reply reply(val);
                                   try {
                                     this->cnet.send_msg(reply,recipient);
                                   } catch(std::exception &err) {
                                     if (DEBUG) std::cout << KBLU << nfo() << "couldn't send reply to " << cid << ":" << reply.prettyPrint() << "; " << err.what() << KNRM << std::endl;
                                   }
                                 } else {
                                   if (DEBUG) std::cout << KBLU << nfo() << "couldn't reply to unknown client: " << cid << KNRM << std::endl;
                                 }
                               }
                               return false;
                             });

  HOST host = "127.0.0.1";
  PORT rport = 8760 + this->myid;
  PORT cport = 9760 + this->myid;

  Nodes nodes("config",this->numNodes);

  NodeInfo* ownnode = nodes.find(this->myid);
  if (ownnode != NULL) {
    host  = ownnode->host;
    rport = ownnode->rport;
    cport = ownnode->cport;
  } else {
    if (DEBUG) std::cout << KLRED << nfo() << "couldn't find own information among nodes" << KNRM << std::endl;
  }

  salticidae::NetAddr paddr = salticidae::NetAddr(host + ":" + std::to_string(rport));
  this->pnet.start();
  this->pnet.listen(paddr);

  salticidae::NetAddr caddr = salticidae::NetAddr(host + ":" + std::to_string(cport));
  this->cnet.start();
  this->cnet.listen(caddr);

  if (DEBUG) std::cout << KBLU << nfo() << "connecting to " << this->numNodes-1 << " nodes..." << KNRM << std::endl;
  for (size_t j = 0; j < this->numNodes; j++) {
    if (this->myid != j) {
      NodeInfo * info = nodes.find(j);
      if (info != NULL) {

        if (DEBUG) std::cout << KBLU << nfo() << "connecting to: " << j << " " << info->host << " " << info->rport << KNRM << std::endl;

        salticidae::NetAddr peer_addr(info->host + ":" + std::to_string(info->rport));
        salticidae::PeerId other{peer_addr};
        this->pnet.add_peer(other);
        this->pnet.set_peer_addr(other, peer_addr);
        this->pnet.conn_peer(other);
        if (DEBUG) std::cout << KBLU << nfo() << "added peer:" << j << KNRM << std::endl;
        this->peers.push_back(std::make_pair(j,other));

      }
    }
  }
  this->pnet.reg_handler(salticidae::generic_bind(&Handler::handle_ping,  this, _1, _2));
  this->cnet.reg_handler(salticidae::generic_bind(&Handler::handle_start, this, _1, _2));

  auto pshutdown = [&](int) {pec.stop();};
  salticidae::SigEvent pev_sigterm(pec, pshutdown);
  pev_sigterm.add(SIGTERM);

  auto cshutdown = [&](int) {cec.stop();};
  salticidae::SigEvent cev_sigterm(cec, cshutdown);
  cev_sigterm.add(SIGTERM);

  c_thread = std::thread([this]() { cec.dispatch(); });

  if (DEBUG) { std::cout << KBLU << nfo() << "dispatching" << KNRM << std::endl; }
  pec.dispatch();
}


std::vector<salticidae::PeerId> getPeerids(Peers recipients) {
  std::vector<salticidae::PeerId> ret;
  for (Peers::iterator it = recipients.begin(); it != recipients.end(); ++it) {
    Peer peer = *it;
    ret.push_back(std::get<1>(peer));
  }
  return ret;
}


void Handler::handle_start(Start msg, const ClientNet::conn_t &conn) {
  if (DEBUG) std::cout << KBLU << nfo() << "handle_start" << KNRM << std::endl;
  CID cid = msg.cid;

  if (this->clients.find(cid) == this->clients.end()) {
    (this->clients)[cid]=conn;
  }

  Ping p(0,cid);
  std::vector<salticidae::PeerId> ps = getPeerids(peers);
  this->pnet.multicast_msg(p, ps);
  if (DEBUG) std::cout << KBLU << nfo() << "sent ping: " << p.prettyPrint() << " to " << ps.size() << " peers" <<KNRM << std::endl;
}


void Handler::handle_ping(Ping msg, const PeerNet::conn_t &conn) {
  if (DEBUG) std::cout << KBLU << nfo() << "handle_ping" << KNRM << std::endl;
  numPings++;
  if (DEBUG) std::cout << KBLU << nfo() << "received " << numPings  << " pings" << KNRM << std::endl;

  // -- Testing the SGX functions --
  COUNT  c  = callTEEadd();
  auth_t a  = callTEEsign("test");
  bool   b1 = callTEEverify("test",a);
  bool   b0 = callTEEverify("foo",a);
  if (DEBUG) std::cout << KBLU << nfo() << "SGX info - counter: " <<  c << "; verify(1):" << b1 << "; verify(0):" << b0 << KNRM << std::endl;
  // -- --

  if (numPings == this->numNodes - 1) {
    // if received pings from all other nodes we send a reply to the client
    if (DEBUG) std::cout << KBLU << nfo() << "received enough pings to reply to the client" << KNRM << std::endl;
    rep_queue.enqueue(std::make_pair(msg.val,msg.cid));
    // and then we stop
    usleep(2000000);
    this->req_tcall->async_call([this](salticidae::ThreadCall::Handle &) { cec.stop(); });
    c_thread.join();
    pec.stop();
  }
}
