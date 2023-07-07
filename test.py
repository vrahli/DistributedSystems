## Imports
from subprocess import Popen
import subprocess
import time
import multiprocessing
import re
from pathlib import Path
import argparse

docker        = "docker"
dockerBase    = "test"  # name of the docker container
useSGX        = False
ncores        = multiprocessing.cpu_count()  # number of cores to use to make
numServers    = 4
numClients    = 1
ipsOfNodes    = {}             # dictionnary mapping node ids to IPs
startRport    = 8760
startCport    = 9760
mybridge      = "testNet"
networkLat    = 0          # network latency in ms
networkVar    = 0          # variation of the network latency
rateMbit      = 0          # bandwidth
dockerMem     = 0          # memory used by containers (0 means no constraints)
dockerCpu     = 0          # cpus used by containers (0 means no constraints)
allLocalPorts = []         # list of all port numbers used in local experiments
srcsgx        = "source /opt/intel/sgxsdk/environment" # this is where the sdk is supposed to be installed
statsdir      = "stats"                                # stats directory (don't change, hard coded in C++)
sgxmode       = "SIM"
addresses     = "config"                               # (don't change, hard coded in C++)

## generates a local config file
def genLocalConf():
    open(addresses, 'w').close()
    host = "127.0.0.1"

    global allLocalPorts

    print("ips:" , ipsOfNodes)

    f = open(addresses,'a')
    for i in range(numServers):
        host  = ipsOfNodes.get(i,host)
        rport = startRport+i
        cport = startCport+i
        allLocalPorts.append(rport)
        allLocalPorts.append(cport)
        f.write("id:"+str(i)+" host:"+host+" port:"+str(rport)+" port:"+str(cport)+"\n")
    f.close()
# End of genLocalConf

def startContainers():
    print("running in docker mode, starting" , numServers, "containers for the severs and", numClients, "for the clients")

    global ipsOfNodes

    lr = list(map(lambda x: (True, x, str(x)), list(range(numServers))))          # replicas
    lc = list(map(lambda x: (False, x, "c" + str(x)), list(range(numClients))))  # clients
    lall = lr + lc + [(False , 0, "x")]

    subprocess.run([docker + " network create --driver=bridge " + mybridge], shell=True)

    for (isServer, j, i) in lall:
        instance  = dockerBase + i
        # We stop and remove the Doker instance if it is still exists
        subprocess.run([docker + " stop " + instance], shell=True) #, check=True)
        subprocess.run([docker + " rm "   + instance], shell=True) #, check=True)
        opt1  = "--expose=" + str(startRport+numServers) if isServer else ""
        opt2  = "--expose=" + str(startCport+numServers) if isServer else ""
        opt3  = "-p " + str(startRport + j) + ":" + str(startRport + j) + "/tcp" if isServer else ""
        opt4  = "-p " + str(startCport + j) + ":" + str(startCport + j) + "/tcp" if isServer else ""
        opt5  = "--network=\"" + mybridge + "\""
        opt6  = "--cap-add=NET_ADMIN"
        opt7  = "--name " + instance
        optm  = "--memory=" + str(dockerMem) + "m" if dockerMem > 0 else ""
        optc  = "--cpus=\"" + str(dockerCpu) + "\"" if dockerCpu > 0 else ""
        opts  = " ".join([opt1, opt2, opt3, opt4, opt5, opt6, opt7, optm, optc]) # with cpu/mem limitations
        if i == "x":
            opts = " ".join([opt1, opt2, opt3, opt4, opt5, opt6, opt7])          # without cpu/mem limitations
        # We start the Docker instance
        subprocess.run([docker + " run -td " + opts + " " + dockerBase], shell=True, check=True)
        subprocess.run([docker + " exec -t " + instance + " bash -c \"" + srcsgx + "; mkdir " + statsdir + "\""], shell=True, check=True)
        # Set the network latency
        if 0 < networkLat:
            print("----changing network latency to " + str(networkLat) + "ms")
            rate = ""
            if rateMbit > 0:
                BUF_PKTS=33
                BDP_BYTES=(networkLat/1000.0)*(rateMbit*1000000.0/8.0)
                BDP_PKTS=BDP_BYTES/1500
                LIMIT_PKTS=BDP_PKTS+BUF_PKTS
                rate = " rate " + str(rateMbit) + "Mbit limit " + str(LIMIT_PKTS)
            #latcmd = "tc qdisc add dev eth0 root netem delay " + str(networkLat) + "ms " + str(networkVar) + "ms distribution normal" + rate
            # the distribution arg causes problems... the default distribution is normal anyway
            latcmd = "tc qdisc add dev eth0 root netem delay " + str(networkLat) + "ms " + str(networkVar) + "ms" + rate
            print(latcmd)
            #latcmd = "tc qdisc add dev eth0 root netem delay " + str(networkLat) + "ms"
            subprocess.run([docker + " exec -t " + instance + " bash -c \"" + latcmd + "\""], shell=True, check=True)
        # Extract the IP address of the container
        ipcmd = docker + " inspect " + instance + " | jq '.[].NetworkSettings.Networks." + mybridge + ".IPAddress'"
        srch = re.search('\"(.+?)\"', subprocess.run(ipcmd, shell=True, capture_output=True, text=True).stdout)
        if srch:
            out = srch.group(1)
            print("----container's address:" + out)
            if isServer:
                ipsOfNodes.update({int(i):out})
        else:
            print("----container's address: UNKNOWN")
## End of startContainers

def stopContainers():
    print("stopping and removing docker containers")

    lr = list(map(lambda x: (True, str(x)), list(range(numServers))))         # servers
    lc = list(map(lambda x: (False, "c" + str(x)), list(range(numClients))))  # clients
    lall = lr + lc + [(False , "x")]

    for (isRep, i) in lall:
        instance = dockerBase + i
        subprocess.run([docker + " stop " + instance], shell=True) #, check=True)
        subprocess.run([docker + " rm "   + instance], shell=True) #, check=True)

    subprocess.run([docker + " network rm " + mybridge], shell=True)
## End of stopContainers

def mkApp():
    # make 1 instance: the "x" instance
    instancex = dockerBase + "x"
    adstx     = instancex + ":/app/App/"
    edstx     = instancex + ":/app/Enclave/"
    subprocess.run([docker + " cp Makefile "  + instancex + ":/app/"], shell=True, check=True)
    subprocess.run([docker + " cp App/. "     + adstx], shell=True, check=True)
    #subprocess.run([docker + " cp Enclave/. " + edstx], shell=True, check=True)
    subprocess.run([docker + " exec -t " + instancex + " bash -c \"make clean\""], shell=True, check=True)
    if useSGX:
        subprocess.run([docker + " exec -t " + instancex + " bash -c \"" + srcsgx + "; make -j " + str(ncores) + " SGX_MODE=" + sgxmode + "\""], shell=True, check=True)
    else:
        subprocess.run([docker + " exec -t " + instancex + " bash -c \"make -j " + str(ncores) + " server client\""], shell=True, check=True)

    tmp = "docker_tmp"
    #Path(tmp).rmdir()
    Path(tmp).mkdir(parents=True, exist_ok=True)
    subprocess.run([docker + " cp " + instancex + ":/app/." + " " + tmp + "/"], shell=True, check=True)

    # copy the files over to the other instances
    lr = list(map(lambda x: str(x), list(range(numServers))))        # Servers
    lc = list(map(lambda x: "c" + str(x), list(range(numClients))))  # Clients
    for i in lr + lc:
        instance = dockerBase + i
        print("copying files from " + instancex + " to " + instance)
        subprocess.run([docker + " cp " + tmp + "/." + " " + instance + ":/app/"], shell=True, check=True)
## End of mkApp

def execute():
    # create config file and copy it over
    genLocalConf()

    lr = list(map(lambda x: str(x), list(range(numServers))))        # servers
    lc = list(map(lambda x: "c" + str(x), list(range(numClients))))  # clients
    lall = lr + lc

    for i in lall:
        dockerInstance = dockerBase + i
        dst = dockerInstance + ":/app/"
        subprocess.run([docker + " cp " + addresses + " " + dst], shell=True, check=True)

    print(">>>>>>>>>>>>>>>>>>> starting servers")
    for i in range(numServers):
        cmd = " ".join(["./server", str(i), str(numServers)])
        dockerInstance = dockerBase + str(i)
        if useSGX:
            cmd = srcsgx + "; " + cmd
        cmd = docker + " exec -t " + dockerInstance + " bash -c \"" + cmd + "\""
        p = Popen(cmd, shell=True)

    time.sleep(20)

    print(">>>>>>>>>>>>>>>>>>> starting clients")
    for i in range(numClients):
        cmd = " ".join(["./client", str(i), str(numServers)])
        dockerInstance = dockerBase + "c" + str(i)
        if useSGX:
            cmd = srcsgx + "; " + cmd
        cmd = docker + " exec -t " + dockerInstance + " bash -c \"" + cmd + "\""
        p = Popen(cmd, shell=True)

    time.sleep(20)
## End of execute


parser = argparse.ArgumentParser(description='test evaluation')
parser.add_argument("--servers", type=int, default=1, help="specifies the number of servers")

args = parser.parse_args()


if args.servers >= 0:
    numServers = args.servers
    print("SUCCESSFULLY PARSED ARGUMENT - the numbers of servers is:", numServers)


# Run the application
startContainers()
mkApp()
execute()
stopContainers()
