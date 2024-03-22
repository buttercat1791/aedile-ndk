<<<<<<< HEAD
# Aedile

A Nostr System Development Kit written in C++.

## Behind the Name

In the ancient Roman Republic, the aediles were officials elected from among the plebians and charged with caring for Rome's public infrastructure and ensuring an accurate system of weights and measures.

The aim of the Aedile SDK is in the spirit of that ancient office:

- Provide a fast and efficient service for interacting with Nostr relays via WebSocket connections.
- Offer stable, well-tested implementations of commonly-used [Nostr Implementation Possibilities (NIPs)](https://github.com/nostr-protocol/nips/tree/master).
- Open up Nostr development by taking care of the basics so developers can focus on solving problems, rather than reimplementing the protocol.

## Building the SDK

### Prerequisites

This project uses CMake as its build system, and vcpkg as its dependency manager.  Thus, to build the SDK, you will need the following:

- CMake 3.19 or later
- A C++17 compiler
- vcpkg

CMake invokes vcpkg at the start of the configure process to install some of the project's dependencies.  For this step to succeed, ensure that the `VCPKG_ROOT` environment variable is set to the path of your vcpkg installation.

### Building and Testing

The SDK aims to support Linux, Windows, and macOS build targets.  It currently supplies a CMake preset for Linux.

#### Linux

To build the SDK on Linux, run the following commands from the project root:

```bash
export VCPKG_ROOT=/path/to/vcpkg/installation
cmake --preset=linux # configuration step
cmake --build build/linux # compilation or build step
```

To run unit tests, use the following command:

```bash
ctest --preset linux
```

### Build using Docker
1. Build docker image:
    docker build -t nostr-ubuntu -f Dockerfile

2. Run docker image
    docker run -it --privileged -u slavehost --hostname slavehost nostr-ubuntu bash


