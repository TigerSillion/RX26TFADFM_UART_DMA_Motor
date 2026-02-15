# UART Rate Tester (GUI)

Simple desktop GUI for UART echo stress test and baud-rate upper-limit scan.

## Requirements
- Python 3.10+
- pyserial (`pip install pyserial`)
- Windows COM port access

## Run
```powershell
python tools\UartRateTester\uart_rate_tester.py
```

## Usage
1. Connect MCU board and open this tool.
2. Select COM port.
3. Keep baud candidates checked (or select subset).
4. Set:
   - `Payload(bytes)` default `512`
   - `Packets/baud` default `100`
   - `ReadTimeout(s)` default `0.50`
5. Click `Start Scan`.
6. Read per-baud results in table:
   - `ok`
   - `sent/recv`
   - `mismatch`
   - `timeout`
   - `kbps`

## Notes
- The test uses MCU command handshake:
  - `@PING`
  - `@SETBAUD:<rate>`
  - `@READY:<rate>`
- Start link must be at `115200`.
- The test assumes firmware echoes received bytes after baud switch.
- On first failure at a baud, this tool marks that baud as fail and continues to next baud.
- Use results to identify the practical upper baud limit for your current firmware and cable setup.
