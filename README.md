# ibkr_gateway_trading_floor

A Windows tray application for Interactive Brokers Gateway.

## Features
- Tray icon with connection status (gray = offline)
- Auto-connects to IB Gateway on startup
- Watchdog reconnection every 10 seconds
- Symbol book — create lists of symbols with autocomplete search
- Window position persistence via registry

## Requirements
- Interactive Brokers Gateway installed
- IBKR Pro account

## Build
Cross-compiled on Linux (Raspberry Pi) targeting Windows x86_64.
```sh
    make lib   # build dependencies (once)
    make       # build TNT.exe
```

## Build-in Dependencies
- IBKR TWS C++ API
- Protobuf + Abseil (via CMake)
- Intel Decimal Floating Point Library

## Usage
- Enjoy!
- To exit `TNT.exe`, look for the Exit menu option in your system tray icon

## Uninstall
- Delete `TNT.exe`
- Open the Registry Editor and delete the folder `Computer\HKEY_CURRENT_USER\Software\ibkr_gateway_trading_floor`.

## External Dependency
- Download and install IBKR Gateway for Windows from https://www.interactivebrokers.com/en/trading/ibgateway-latest.php
