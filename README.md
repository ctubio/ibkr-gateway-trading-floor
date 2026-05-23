# IBKR Gateway: Trading Floor

A lightweight Windows tray application for real-time trading and market data from Interactive Brokers Gateway. Provides instant access to account information, positions, orders, market data, and trading tools—all from your system tray.

## Overview

A modern desktop trading companion built on the Interactive Brokers TWS C++ API. It connects to IBKR Gateway (not Trader Workstation) and provides multiple synchronized windows for monitoring and managing your account in real-time.

### Key Highlights

- **Zero-footprint tray application** — Runs minimally in your system tray with status indicators
- **Auto-connect & watchdog** — Automatically connects on startup and recovers from disconnections
- **Real-time data** — Market data and trading updates via tick-by-tick feeds
- **Multi-window layout** — Organize separate windows for different trading views, with persistent positions/sizing
- **Account & P&L monitoring** — Live account summary with daily/unrealized/realized P&L
- **Position management** — Track open positions with average cost, daily P&L, and market value
- **Order tracking** — Monitor all open and completed orders with fill status
- **Symbol search** — Quick symbol lookup with contract details
- **Market news** — Real-time news headlines for any symbol
- **Tick-by-tick data** — Time & Sales window with complete trade history
- **Book management** — Create and organize custom symbol watchlists
- **Settings panel** — Persistent, registry-backed configuration

## Features by Module

### Dashboard
Main window showing connection status (Market Data ✓/✗, Trading ✓/✗) and account number. Tray icon displays this same status with color-coding for online/offline.

### Coins (Account Summary)
Real-time account metrics:
- Net Liquidation Value
- Cash Balances (Base, EUR, USD)
- Buying Power & Available Funds
- Gross Position Value & Excess Liquidity
- Margin Requirements (Init & Maint)
- **P&L Dashboard**: Daily, Unrealized, and Realized profit/loss

### Diamonds (Portfolio)
Your current positions with:
- Symbol and quantity
- Average cost per share
- Daily P&L (mark-to-market)
- Market value of position
- 52-week change & market cap

### Orders
Complete order ledger:
- Open orders (Submitted, PreSubmitted, Partially Filled)
- Filled orders (recent completions)
- Cancelled orders
- Status, fill price, and timestamps
- Sorted by status priority and newest first

### Time & Sales
Real-time tick-by-tick trade data:
- Price and size of each trade
- Exchange source
- High-precision timestamps (HH:MM:SS)
- Streaming updates as trades occur

### Book (Symbol Watchlist)
Create and manage custom symbol lists:
- Add/remove symbols dynamically
- Organize trading ideas by category
- Quick navigation to symbol details

### Ticker & Levels
Market depth and pricing:
- Bid/ask levels
- Order book visualization
- Real-time quote updates

### News
Symbol-specific news headlines:
- Provider source
- Real-time news delivery
- Extra context/details per article

### Settings
Configuration panel for app preferences (persistent via registry)

### Debug Log
Internal diagnostics and API event logging (for troubleshooting)

## Requirements

### Software
- **Windows 10/11** (x86_64)
- **IBKR Pro account** (market data requires professional subscription)
- **IBKR Gateway** (not Trader Workstation)
  - Download from: https://www.interactivebrokers.com/en/trading/ibgateway-latest.php

### System
- Network connectivity to IBKR servers
- ~20 MB disk space for the executable and settings

## Installation

1. Download the latest `Trading-Floor.exe` release
2. Place it anywhere on your system (e.g., `C:\Program Files\TNT\`)
3. Run `Trading-Floor.exe` — it will appear in your system tray
4. Launch IBKR Gateway and authenticate
5. TNT will auto-connect within seconds

## Configuration

TNT stores all settings in the Windows Registry under:
```
HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor
```

- **Window positions and sizes** are automatically saved on close
- **User preferences** are persisted via the Settings panel
- Delete this registry folder to reset TNT to defaults

## Build Instructions

**TNT is cross-compiled on Linux (Raspberry Pi) targeting Windows x86_64.**

### Prerequisites
- `x86_64-w64-mingw32-g++` and related MinGW tools
- `cmake` (for building Protobuf)
- Standard build tools (`make`, `sed`, `awk`, etc.)

### Build Steps

```sh
# First-time setup: compile all dependencies (one-time, ~5-10 min)
make lib

# Build the TNT executable
make

# Output: build/Trading-Floor.exe
```

### Build-in Dependencies

The project includes pre-vendored dependencies:

- **IBKR TWS C++ API** (`lib/CppClient/`)
  - Official Interactive Brokers API for C++
  - Contract, Order, OrderState definitions
  - EWrapper callbacks for all market/trading events

- **Protobuf + Abseil** (`lib/protobuf/`, built via CMake)
  - Protobuf serialization library
  - Abseil C++ Standard Library extensions
  - UTF-8 range utilities

- **Intel Decimal Floating Point Library** (`lib/IntelRDFPMathLib20U4/`)
  - Precise decimal arithmetic for financial calculations
  - Avoids floating-point rounding errors

## Architecture

```
src/
├── main.cpp               # WinMain entry point, window registration
├── api.h                  # TradingAPI class: IBKR connection & EWrapper implementation
├── dashboard.h            # Dashboard window (status/tray)
├── coins.h                # Account summary & P&L
├── diamonds.h             # Portfolio/positions
├── orders.h               # Order tracking
├── timesales.h            # Tick-by-tick trade data
├── ticker.h               # Market quotes/depth
├── levels.h               # Order book levels
├── book.h                 # Symbol watchlist
├── news.h                 # News headlines
├── settings.h             # Configuration UI
├── shared.h               # Common window utilities, registry helpers
├── registry.h             # Windows Registry API wrappers
├── debug.h                # Logging & diagnostics
└── sound.h                # Audio feedback

lib/
├── CppClient/             # IBKR TWS C++ API
├── protobuf/              # Protobuf library
└── IntelRDFPMathLib20U4/  # Decimal arithmetic library

build/
└── Trading-Floor.exe      # Final executable
```

## Usage
 - Enjoy!
 - To exit `Trading-Floor.exe`, look for the Exit menu option in your system tray icon

## Uninstall
- Delete `Trading-Floor.exe`
- Open **Registry Editor** (`regedit.exe`) and delete the folder `Computer\HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor`

## Performance Notes

- **Memory**: Typically 30–50 MB resident
- **CPU**: Minimal when idle (<1%), spikes only during data updates
- **Network**: <5KB/sec typical during active trading
- **Latency**: Sub-100ms from Gateway API callback to UI update

## License & Legal

- This project is provided as-is for educational and personal trading use
- Interactive Brokers® and IB Gateway® are registered trademarks of Interactive Brokers
- Ensure compliance with Interactive Brokers' API usage terms
- Use at your own risk; test thoroughly before live trading
