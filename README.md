# NostrSDK

C++ System Development Kit for Nostr

## Building the SDK

### Prerequisites

This project uses CMake as its build system, and vcpkg as its dependency manager.  Thus, to build the SDK, you will need the following:

- CMake 3.19 or later
- C++17 compiler
- vcpkg

### Build Targets

The SDK aims to support Linux, Windows, and macOS build targets.  CMake presets are provided for each target.

#### Linux

To build the SDK on Linux, run the following commands from the project root:

```bash
cmake --preset=linux .
cmake --build ./build/linux
```
