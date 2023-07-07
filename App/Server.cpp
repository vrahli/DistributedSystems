#include <iostream>


#include "Message.h"
#include "Handler.h"


int main(int argc, char const *argv[]) {

  unsigned int myid = 0;
  if (argc > 1) { sscanf(argv[1], "%d", &myid); }
  std::cout << KYEL << "[id=" << myid << "]" << KNRM << std::endl;

  unsigned int numNodes = 1;
  if (argc > 2) { sscanf(argv[2], "%d", &numNodes); }
  std::cout << KYEL << "[" << myid << "]#nodes=" << numNodes << KNRM << std::endl;


  // Initializing handler
  unsigned int size = std::max({sizeof(Start), sizeof(Ping), sizeof(Reply)});
  PeerNet::Config pconfig;
  pconfig.max_msg_size(2*size);
  //pconfig.nworker(4);
  //pconfig.burst_size(1000);
  ClientNet::Config cconfig;
  cconfig.max_msg_size(2*size);
  Handler handler(myid,numNodes,pconfig,cconfig);

  return 0;
};
