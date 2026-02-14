# Changelog

## 2026-02-14

### Added
- UART-DMA MVP app layer: `src/app/common/*`, `src/app/uart_dma/*`.
- Middleware documentation: `docs/MIDDLEWARE_FLOW_UART_DMA.md`.
- SMC modification documentation: `docs/SMC_GEN_MODIFICATIONS.md`.
- Engineering rule documentation: `docs/ENGINEERING_RULES.md`.
- SMC regeneration checklist: `docs/SMC_REGEN_CHECKLIST.md`.
- Change governance files under `.codex/rules` and skill/docs sync under `.agents`.

### Changed
- Main entry now runs UART-DMA demo loop in `src/RX26TFADFM_UART_DMA_Motor.c`.
- `smc_gen` user regions updated for SCI5/DMAC0/DMAC1 callback and address bridge.
- `AGENTS.md` and `.codex/rules/*.md` now require mandatory change logs and test evidence.

### Notes
- `smc_gen` non-user generated regions were not modified.
- Full project link build not executed in this shell due missing `make`; key files compiled by `ccrx` successfully.
