#include <iostream>
#include <cstring>
#include <fstream>

#include "Nodes.h"

Nodes::Nodes(std::string filename, unsigned int numNodes) {
  std::ifstream inFile(filename);
  char oneline[MAXLINE];
  char delim[] = " ";
  char *token;
  unsigned int added = 0;

  if (DEBUG) std::cout << KMAG << "parsing configuration file" << KNRM << std::endl;

  while (inFile && added < numNodes) {
    inFile.getline(oneline,MAXLINE);
    token = strtok(oneline,delim);

    if (token) {
      // id
      int id = atoi(token+3);

      // host
      token=strtok(NULL,delim);
      HOST host = token+5;

      // replica port
      token=strtok(NULL,delim);
      PORT rport = atoi(token+5);

      // client port
      token=strtok(NULL,delim);
      PORT cport = atoi(token+5);

      if (DEBUG) std::cout << KMAG << "id: " << id << "; host: " << host << "; port: " << rport << "; port: " << cport << KNRM << std::endl;

      NodeInfo hp(id,host,rport,cport);
      nodes[id]=hp;
      added++;
    }
  }

  if (DEBUG) std::cout << KMAG << "closing configuration file" << KNRM << std::endl;
  inFile.close();
  if (DEBUG) std::cout << KMAG << "done parsing the configuration file" << KNRM << std::endl;
}


NodeInfo * Nodes::find(PID id) {
  std::map<PID,NodeInfo>::iterator it = nodes.find(id);

  if (it != nodes.end()) {
    if (DEBUG) { std::cout << KMAG << "found a corresponding NodeInfo entry" << KNRM << std::endl; }
    return &(it->second);
  } else { return NULL; }
}
