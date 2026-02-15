@echo off
setlocal
cd /d %~dp0\..\..
python tools\UartRateTester\uart_rate_tester.py
