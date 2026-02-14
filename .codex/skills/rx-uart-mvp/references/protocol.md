# Protocol Baseline

- Byte-oriented UART stream.
- MVP phase uses single-byte echo and command placeholders.
- Future phase extends to framed packets (SOF, LEN, PAYLOAD, CRC).
- Parser must recover from corrupted input by resync scanning.
