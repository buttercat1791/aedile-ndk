# NostrSDK

C++ System Development Kit for Nostr

## Feature Roadmap

### Basic Nostr Client Support

- [x] Write to relays via WebSocket.
- [ ] Read from relays via WebSocket.
- [ ] Sign events via remote signer as per [NIP-46](https://github.com/nostr-protocol/nips/blob/master/46.md).
- [ ] Derive keypairs according to [NIP-06](https://github.com/nostr-protocol/nips/blob/master/06.md).
- [ ] Implement [NIP-01](https://github.com/nostr-protocol/nips/blob/master/01.md) kinds, tags, and messages.

### Value-for-Value

- [ ] Use [Nostr Wallet Connect](https://github.com/nostr-protocol/nips/blob/master/47.md) to connect to Lightning wallets.
- [ ] Send and receive [Zaps](https://github.com/nostr-protocol/nips/blob/master/57.md).

### Metadata Handling

- [ ] Read [profile data](https://github.com/nostr-protocol/nips/blob/master/24.md#kind-0) from kind 0 events.
- [ ] Use [kind 10002](https://github.com/nostr-protocol/nips/blob/master/65.md) events for relay metadata.
- [ ] Read and update [follow lists](https://github.com/nostr-protocol/nips/blob/master/02.md).
- [ ] Read and update additional [list types](https://github.com/nostr-protocol/nips/blob/master/51.md).
- [ ] Handle [media attachments](https://github.com/nostr-protocol/nips/blob/master/92.md).
- [ ] Support [file storage over HTTP](https://github.com/nostr-protocol/nips/blob/master/96.md).

### Additional Social Interactions

- [ ] Mark events for [deletion](https://github.com/nostr-protocol/nips/blob/master/09.md).
- [ ] Support [sensitive content](https://github.com/nostr-protocol/nips/blob/master/36.md) filtering.
- [ ] Support event [reporting](https://github.com/nostr-protocol/nips/blob/master/56.md).
- [ ] Create and read [repost events](https://github.com/nostr-protocol/nips/blob/master/18.md).
- [ ] Organize replies into threads per [NIP-10](https://github.com/nostr-protocol/nips/blob/master/10.md).
- [ ] Send and read [reactions](https://github.com/nostr-protocol/nips/blob/master/25.md).
- [ ] Create shareable links using encoding defined in [NIP-19](https://github.com/nostr-protocol/nips/blob/master/19.md).

### Authentication and Security

- [ ] Allow [HTTP Authentication](https://github.com/nostr-protocol/nips/blob/master/98.md) with servers.

### Builds and Integrations

- [ ] Dockerize the build process for Windows, Linux, and macOS targets.
- [ ] Define gRPC interfaces for cross-language client support.
