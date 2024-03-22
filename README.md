# NostrSDK
C++ System Development Kit for Nostr

## Build using Docker
1. Build docker image:
    docker build -t nostr-ubuntu -f Dockerfile

2. Run docker image
    docker run -it --privileged -u slavehost --hostname slavehost nostr-ubuntu bash

3. Once inside the docker container, navigate to the NostrSDk directory and execute the compiled test.
