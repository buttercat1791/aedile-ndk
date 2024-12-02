# Test Scenarios for `nostr::data`

## NIP-01: Nostr Event Format

### 1. Idempotent ID Generation

    GIVEN 2 events with the same values for `pubkey`, `created_at`, `kind`, `tags`, and `content`
    WHEN the `pubkey`, `created_at`, `kind`, `tags`, and `content` are arranged in a JSON array of the form `[0,pubkey,created_at,kind,tags,content]` to generate the events' IDs
    THEN the IDs generated for both events are identical

### 2. Escaping Special Characters in Content

    GIVEN the content field of an event contains any of the characters `\b`, `\t`, `\n`, `\f`, `\r`, `"`, or `\`
    WHEN the content is serialized for ID generation
    THEN the special characters are included verbatim in the resulting string, with proper escape sequences.
