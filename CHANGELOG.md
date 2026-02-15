# Changelog

## 2026-02-14

### Added
- UART-DMA MVP app layer: `src/app/common/*`, `src/app/uart_dma/*`.
- Middleware documentation: `docs/MIDDLEWARE_FLOW_UART_DMA.md`.
- SMC modification documentation: `docs/SMC_GEN_MODIFICATIONS.md`.
- Engineering rule documentation: `docs/ENGINEERING_RULES.md`.
- SMC regeneration checklist: `docs/SMC_REGEN_CHECKLIST.md`.
- Change governance files under `.codex/rules` and skill/docs sync under `.agents`.
- GUI UART rate-limit tester:
  - `tools/UartRateTester/uart_rate_tester.py`
  - `tools/UartRateTester/README.md`
  - `tools/UartRateTester/run_uart_rate_tester.bat`

### Changed
- Main entry now runs UART-DMA demo loop in `src/RX26TFADFM_UART_DMA_Motor.c`.
- `smc_gen` user regions updated for SCI5/DMAC0/DMAC1 callback and address bridge.
- `AGENTS.md` and `.codex/rules/*.md` now require mandatory change logs and test evidence.

### Notes
- `smc_gen` non-user generated regions were not modified.
- Full project link build not executed in this shell due missing `make`; key files compiled by `ccrx` successfully.

### Fixed
- Rewrote garbled Chinese documents with UTF-8 encoding:
  - `docs/SMC_GEN_MODIFICATIONS.md`
  - `docs/MIDDLEWARE_FLOW_UART_DMA.md`
  - `docs/ENGINEERING_RULES.md`

### Improved
- Added TX timeout protection in `uart_dma_port_tx_async()` to prevent endless blocking on TEI timeout.
- Added UART diagnostics counters (`rx/tx/tx_timeout/rx_error`) in `uart_dma_diag_t`.
- Added interactive UART test interface commands in `src/app/uart_dma/uart_dma_demo.c`:
  - `h` help
  - `s` stats
  - `t` test token
  - `b` burst test
  - `c` clear counters
  - `e` toggle echo
- Added MCU runtime baud control command interface:
  - `@PING`
  - `@GETBAUD`
  - `@SETBAUD:<rate>`
- Updated GUI tester to do staged baud handshake:
  - Start at `115200`
  - Switch baud with command
  - Validate `@READY:<rate>`
  - Run throughput/error test up to `4Mbps` candidate
- Enhanced GUI process visibility in `tools/UartRateTester/uart_rate_tester.py`:
  - Added real-time phase/status panel (`Handshake`, `Switch Baud`, `Stress Test`)
  - Added determinate progress bar
  - Added per-baud packet progress detail (`pkt x/y`)
  - Added `4000000` to default baud candidates
- Fixed GUI scan logic and protocol robustness in `tools/UartRateTester/uart_rate_tester.py`:
  - Fixed handshake gate bug (`"N"` was treated as truthy and did not stop flow)
  - Added robust token-based line wait for `@PONG/@ACK/@READY`
  - Added safe serial open helper and control-line handling to reduce reset-side effects
- Added CSV result persistence for UART GUI tester:
  - Added `Export CSV` button
  - Auto-saves scan results to `tools/UartRateTester/results/*.csv` at end of scan
- Fixed UART GUI command terminators:
  - `@PING` and `@SETBAUD` now send real newline (`\n`) instead of literal `\\n`
  - Resolves command parser not completing on MCU side
- Improved test stability and diagnostics:
  - MCU RX path changed to ring buffer (`UART_DMA_RX_FIFO_SIZE=256`) to avoid single-byte flag overwrite loss
  - Added `rx_overrun_count` diagnostic field and stats output
  - GUI handshake now sends a pre-sync newline to recover MCU command parser state
  - GUI payload pattern constrained to command-safe bytes for echo stress
  - GUI baud-switch now accepts `ACK + @PING(new baud)` as success fallback when `@READY` is missed
- Improved MCU runtime baud switching strategy:
  - `uart_dma_port_set_baud()` now searches SCI baud modes (SMR/SEMR/BRR) and selects the minimum error option
  - Added baud error guard (`UART_DMA_BAUD_ERR_MAX_PERMILLE`)
- Added descending high-to-low loss scan tool:
  - `tools/UartRateTester/uart_desc_loss_scan.py`
  - supports `start/end/step` baud sweep (e.g. `2.5M -> 100k` at `100k` step)
- Added MCU-side auto sweep mode in UART demo command channel:
  - `@AUTOSWEEP:START,END,STEP,MS`
  - `@AUTOSWEEP:STOP`
  - `@AUTOSWEEP:STATUS`
- Added stable-point scan utility:
  - `tools/UartRateTester/uart_stable_point_scan.py`
  - deterministic multi-round validation for `2M~4M` style high-baud candidates
