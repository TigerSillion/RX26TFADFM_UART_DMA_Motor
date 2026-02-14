---
name: rx-uart-mvp
description: Use when implementing or optimizing Renesas RX UART DMA MVP, including framing, throughput tuning, and PC-MCU interoperability diagnostics.
---

# RX UART MVP Skill

## When to use
- RX MCU UART + DMAC design or implementation.
- Framing, variable transfer, and throughput tuning.

## Workflow
1. Confirm transport assumptions: baud, frame size, timer source.
2. Apply frame contract from `references/protocol.md`.
3. Validate parser resilience and resync behavior.
4. Run throughput checks from `references/benchmarks.md`.
5. Apply SMC safety rules from `references/smc_safety.md`.

## Required outputs
- Protocol compatibility statement.
- Throughput summary with test conditions.
- Next bottleneck and optimization step.
