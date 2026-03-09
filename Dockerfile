# Use an official ocaml runtime as a parent image
# Install jammy
FROM ubuntu:22.04

EXPOSE 80/tcp

# Set the working directory to /app
WORKDIR /app

# This is to avoid interactive questions from tzdata
ENV DEBIAN_FRONTEND=noninteractive

# install some requirements
RUN apt-get update \
    && apt-get install -y apt-utils git wget iptables libdevmapper1.02.1 \
    && apt-get install -y build-essential ocaml ocamlbuild automake autoconf libtool python-is-python3 libssl-dev git cmake perl \
    && apt-get install -y libssl-dev libcurl4-openssl-dev protobuf-compiler libprotobuf-dev debhelper cmake reprepro unzip pkgconf libboost-dev libboost-system-dev libboost-thread-dev lsb-release libsystemd0 \
    && apt-get install -y libcurl4-openssl-dev protobuf-compiler libprotobuf-dev debhelper reprepro unzip build-essential python3 \
    && apt-get install -y lsb-release software-properties-common pkg-config libuv1-dev python3-matplotlib emacs psmisc jq iproute2 stress-ng

# clone sgx sdk
RUN mkdir /opt/intel \
    && cd /opt/intel \
    && git clone https://github.com/intel/linux-sgx

# build sdk
RUN cd /opt/intel/linux-sgx \
    && make preparation \
    && make sdk \
    && make sdk_install_pkg
# Do we need this even though this is an ubuntu 22.04?
# Does it need to be right after make preparation?
#    && cp /opt/intel/linux-sgx/external/toolset/ubuntu20.04/* /usr/local/bin \

# install SDK
RUN cd /opt/intel/linux-sgx/linux/installer/bin \
    && echo -e "no\n/opt/intel\n" | ./"$(ls *.bin)"
#    The file name changes from one version to another
#    && echo -e "no\n/opt/intel\n" | ./sgx_linux_x64_sdk_2.25.100.3.bin

# build PSW
RUN mkdir /etc/init
RUN . /opt/intel/sgxsdk/environment \
    && cd /opt/intel/linux-sgx \
    && make psw \
    && make deb_psw_pkg \
    && make deb_local_repo
RUN echo "deb [trusted=yes arch=amd64] file:/opt/intel/linux-sgx/linux/installer/deb/sgx_debian_local_repo jammy main" >> /etc/apt/sources.list
RUN echo "# deb-src [trusted=yes arch=amd64] file:/opt/intel/linux-sgx/linux/installer/deb/sgx_debian_local_repo jammy main" >> /etc/apt/sources.list
RUN apt update

# install psw
#ARG CACHE_BUST=1
#RUN apt-cache search libsgx
#RUN dpkg -l | grep emacs
#RUN dpkg -l | grep libsgx
#RUN apt-get install -y libsgx-launch libsgx-epid
RUN apt-get install -y libsgx-urts libsgx-quote-ex libsgx-dcap-ql

RUN cd /opt/intel/linux-sgx/external/dcap_source/QuoteVerification/sgxssl/Linux \
    && ./build_openssl.sh \
    && make all \
    && make install

# Copy the current directory contents into the container at /app
COPY Makefile       /app
COPY test.py        /app
COPY enclave.token  /app
COPY App            /app/App
COPY Enclave        /app/Enclave
COPY Include        /app/Include

RUN cd /app \
    && git clone https://github.com/Determinant/salticidae.git \
    && cp Include/crypto.h salticidae/include/salticidae/ \
    && cd salticidae \
    && cmake . -DCMAKE_INSTALL_PREFIX=. \
    && make \
    && make install \
    && pwd


#RUN . /opt/intel/sgxsdk/environment && make


CMD ["/bin/bash"]
#ENTRYPOINT ["pwd"]


#### Once docker has been installed, run the following so that sudo won't be needed:
####     sudo groupadd docker
####     sudo gpasswd -a $USER docker
####     -> log out and back in, and then:
####     sudo service docker restart
#### build container: docker build -t test .
#### run container: docker run -td --expose=8000-9999 --network="host" --name test0 test
#### (alternatively) run container in interactive mode: docker container run -it test /bin/bash
#### docker exec -t test0 bash -c "source /opt/intel/sgxsdk/environment; make"


#### This was tested with the sgx-sdk Commit 5e63c02
