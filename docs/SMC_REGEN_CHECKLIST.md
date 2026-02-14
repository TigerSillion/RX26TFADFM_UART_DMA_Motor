# SMC Regeneration Checklist

## Purpose
Protect user logic from Smart Configurator regeneration overwrite.

## Protected files in this project
- `src/smc_gen/Config_SCI5/Config_SCI5_user.c`
- `src/smc_gen/Config_DMAC0/Config_DMAC0_user.c`
- `src/smc_gen/Config_DMAC1/Config_DMAC1_user.c`
- `src/smc_gen/general/r_cg_userdefine.h`

## Rules
- Edit only inside `/* Start user code ... */` regions.
- Keep all business logic under `src/app/*`.
- Mark new protected-region code with `USER CODE SAFE` comments.

## Regeneration flow
1. Update `.scfg` and regenerate by Smart Configurator.
2. Build Debug target.
3. Verify `USER CODE SAFE` snippets still exist in protected files.
4. Run UART echo smoke test and 10,000-byte stress test.

## Quick integrity checks
- Search protected tags:
  - `rg -n "USER CODE SAFE" src/smc_gen -S`
- Search key symbols:
  - `rg -n "g_sci5_rx_byte|uart_dma_port_on_rx_end_isr|uart_dma_port_on_tx_end_isr" src -S`
