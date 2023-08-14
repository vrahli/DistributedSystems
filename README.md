# Base code for coding distributed systems

This is a simple code base for coding distributed systems that relies
on [Salticidae](https://github.com/Determinant/salticidae) for the
communication, on
[netem](https://wiki.linuxfoundation.org/networking/netem) to
simiulate different network latencies, and on
[Docker](https://www.docker.com/) to run the code in containers. It
runs a simple client/server application, with the servers pinging each
other.

## Requirements

This requires installing Docker, jq, python.

This [page](https://docs.docker.com/engine/install/) explains how to
install Docker. In particular follow the following
[instructions](https://docs.docker.com/engine/install/linux-postinstall/)
so that you can run Docker as a non-root user.

## Usage

Create the Docker image:

`docker build -t test .`

Run

`python3 test.py --servers N`

where `N` is the number of servers you want to use.
