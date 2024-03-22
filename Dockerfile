# syntax=docker/dockerfile:1

FROM ubuntu:22.04


# install build tools
RUN apt-get update 
RUN apt-get -y install cmake && apt-get -y install build-essential
RUN apt-get -y install git

# install dependencies
RUN apt-get -y install libboost-all-dev
RUN apt-get -y install libssl-dev
RUN apt-get -y install libwebsocketpp-dev

# create slavehost user
RUN useradd -ms /bin/bash slavehost
RUN usermod -aG sudo slavehost

WORKDIR /home/slavehost

# install plog dependency
RUN git clone https://github.com/SergiusTheBest/plog.git
WORKDIR /home/slavehost/plog
RUN mkdir build
WORKDIR /home/slavehost/plog/build
RUN cmake ..
RUN make
RUN make install

# return to user dir
WORKDIR /home/slavehost

# install nlohmann json
RUN git clone https://github.com/nlohmann/json.git
WORKDIR /home/slavehost/json
RUN mkdir build
WORKDIR /home/slavehost/json/build
RUN cmake ..
RUN make
RUN make install

# return to user dir
WORKDIR /home/slavehost

# change user to slavehost
USER slavehost

# create workspace dir
RUN mkdir -p workspace/nostrsdk

# Add source files to docker container
COPY include /home/slavehost/workspace/nostrsdk/include
COPY src /home/slavehost/workspace/nostrsdk/src
COPY test /home/slavehost/workspace/nostrsdk/test
COPY .gitignore /home/slavehost/workspace/nostrsdk/.gitignore
COPY CMakeLists.txt /home/slavehost/workspace/nostrsdk/CMakeLists.txt

WORKDIR /home/slavehost/workspace/nostrsdk/

RUN mkdir build
WORKDIR /home/slavehost/workspace/nostrsdk/build
RUN cmake ..
RUN make

WORKDIR /home/slavehost
