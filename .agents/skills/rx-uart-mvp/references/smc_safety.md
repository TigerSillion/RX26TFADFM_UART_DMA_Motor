# SMC Safety Reference

- Only modify `smc_gen` user code regions.
- Keep register and transfer policy in application layer under `src/app/*`.
- After `.scfg` regeneration, run compile and user-region integrity checks.
