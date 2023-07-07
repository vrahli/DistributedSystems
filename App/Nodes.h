#ifndef NODES_H
#define NODES_H

#include <map>

#include "config.h"

struct NodeInfo {
  PID pid;
  HOST host;
  PORT rport; // replica port to listen to replicas
  PORT cport; // client port to listen to clients
  NodeInfo() : pid(0),host(""),rport(0),cport(0) {};
  NodeInfo(const PID &pid, const HOST &host, const PORT &rport, const PORT &cport)
    : pid(pid),host(host),rport(rport),cport(cport) {};
};

class Nodes {

  private:
    std::map<PID,NodeInfo> nodes;

  public:
    Nodes(std::string filename, unsigned int numNodes);

    NodeInfo * find(PID id);

};

#endif
