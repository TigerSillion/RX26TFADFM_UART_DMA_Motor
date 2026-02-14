# Throughput Benchmarks

- Minimum test: 10,000 bytes continuous RX/TX echo.
- Record baud rate, payload size, host tool, and error count.
- Track CPU lockup symptoms and ISR starvation.
- Keep single-byte transfer path as baseline before batching.
