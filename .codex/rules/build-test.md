# Build/Test Rule

- Prefer smallest validation first.
- For firmware updates: compile Debug build before hardware verification.
- On failure, report command, key error lines, and immediate mitigation.
- For each validation run, append test evidence to `docs/CHANGE_RECORDS.md`:
  - date/time
  - command
  - pass/fail
  - key log summary
