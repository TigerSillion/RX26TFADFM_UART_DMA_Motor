# RX26TFADFM UART-DMA AGENTS

## Scope
- This file defines project-level behavior for this repository only.
- Runtime knobs and MCP setup live in `.codex/config.toml`.

## Workflow
- Follow: Explore -> Plan -> Implement -> Validate -> Report.
- Keep changes minimal, testable, and directly tied to requested scope.
- Every code/config/doc change must include update notes in `CHANGELOG.md` and `docs/CHANGE_RECORDS.md`.

## SMC Safety
- `src/smc_gen/*` non-user regions are generator-owned.
- Only edit `/* Start user code ... */` protected sections in `smc_gen`.
- Put application logic under `src/app/*`.
- Keep `docs/SMC_GEN_MODIFICATIONS.md` updated when any `smc_gen` user region changes.

## Command Safety
- Never run destructive commands without explicit user approval.
- Forbidden unless explicitly requested: `git reset --hard`, `git checkout --`.
- Use `rg` first for search.

## Review Standard
- For review tasks, output defects first, ordered by severity.
- Include file path and line for each issue.

## Build and Validation
- Start with the smallest relevant build/test command.
- Report exact command and key output when failures occur.
- Record test evidence (command, date/time, result, key logs) in `docs/CHANGE_RECORDS.md`.

## MCP Priority
- For OpenAI API/Codex/ChatGPT product questions, query `openaiDeveloperDocs` MCP first.
