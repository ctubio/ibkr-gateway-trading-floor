# 💼 IBKR Gateway: Trading Floor

A lightweight Windows tray application that connects to Interactive Brokers Gateway and surfaces live account, market, and trading data in a compact, portable Windows GUI.

> System Tray Icon: 💼

## Overview

This application is a modern desktop trading companion built on the Interactive Brokers C++ API. It is designed to work with **IBKR Gateway** (not Trader Workstation) and provides:

- real-time market data and trade updates
- live account summary and margin metrics
- open positions with P&L analytics
- order tracking and status coloring
- symbol watchlists, book management, and quick search
- news headlines with article preview support
- tick-by-tick Time & Sales with filtering
- flexible multi-window layout with persistent state
- dark mode, sound alerts, and debug logging

## Key Highlights

- **Tray-icon**: runs from the system tray with a connected/disconnected icon and always-on-top menu options for each window.
- **Auto-connect watchdog**: monitors connection status and reconnects to IBKR Gateway automatically.
- **IBKR Gateway support**: connects to `127.0.0.1:4001` using the IBKR socket API.
- **Persistent settings**: saved under `HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor`.
- **Multi-window trade desk**: open Dashboard, Coins, Diamonds, Orders, Ticker, News, Time & Sales, Book, Settings, and Debug Log windows.
- **Custom watchlists**: store multiple symbol watchlists in the registry and reuse them across sessions.
- **Dark mode + sounds**: optional dark theme and notification sounds for connection events.

## Core Modules

### Dashboard

- connection summary for Market Data and Trading
- account number display
- tray icon state updates
- quick buttons to open every major window
- periodic watchdog timer that keeps the app connected

### Coins (Account Summary)

Displays live account summary values from IBKR:

- Net Liquidation Value
- Total Cash and currency balances
- Buying Power and Available Funds
- Gross Position Value
- Excess Liquidity
- Initial and Maintenance Margin Requirements
- Daily, Unrealized, and Realized P&L

### Diamonds (Portfolio)

Portfolio window with position details:

- instrument symbol
- quantity and average price
- current bid/ask/last price
- daily P&L and change percentage
- unrealized P&L and unrealized P&L %
- market value and percent of net liquidation
- dividend yield, next dividend date, and annual dividend

### Orders

Tracks open and completed orders with rich status data:

- time, symbol, action, type, quantity
- filled quantity and average fill price
- status coloring for filled/partial/cancelled states
- exchange and price details
- sorted primarily by status and secondarily by newest updates

### Ticker & Levels

Live quote watchlist support:

- last price, bid/ask, and sizes
- change % from previous close
- dividend yield and next dividend details
- fundamental tick updates for 52-week range and market cap
- custom watchlist subscription through saved book lists

### Book (Symbol Watchlist)

Manage reusable watchlists:

- create and delete named lists
- add and remove symbols
- persist lists across restarts
- use saved lists for Ticker, News, and Time & Sales workflows

### News

News viewer with symbol-specific headlines:

- provider code and headline display
- historical + live news for selected symbols
- double-click article download and preview
- supports plain text article rendering in RichEdit

### Time & Sales

Tick-by-tick trade feed per symbol:

- live trade price, size, time, and exchange
- filtered views for large trades (size >=100 and >=1000)
- supports multiple open Time & Sales windows
- symbol search popup for quick instrument selection

### Settings

Persistent application settings saved to the registry:

- Auto-start IBKR Gateway
- Kill Gateway on exit
- Dark mode
- Play sounds
- Debug log access

### Debug Log

Internal runtime diagnostics and API event logging:

- captures heartbeat, connection, API error, and message events
- displays buffered logs while the window is closed
- useful for troubleshooting connectivity and IBKR message flow

## Requirements

### Software

- Windows 10 or 11 (x86_64)
- IBKR Gateway
- IBKR account with market data permissions

### System

- TCP connectivity to IBKR Gateway/servers
- ~20 MB free disk space for executable and settings

## Installation

1. Download the latest `Trading-Floor.exe` build.
2. Place it anywhere on your machine, for example `C:\Program Files\TNT\`.
3. Run `Trading-Floor.exe`.
4. Start and authenticate IBKR Gateway.
5. The app will auto-connect within seconds when Gateway is available.

> Tip: enable **Auto-start IBKR Gateway** in Settings to have the app launch Gateway automatically if it is not already running.

## Configuration

All app settings and state are stored under:

```
HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor
```

Saved state includes:

- window size/position
- watchlist definitions
- last selected news list/symbol
- open Time & Sales sessions
- dark mode and sound preferences
- auto-gateway and kill-gateway options

## Build Instructions

This project is designed to be cross-built from Linux using MinGW.

### Prerequisites

- `x86_64-w64-mingw32-g++`
- `x86_64-w64-mingw32-gcc`
- `x86_64-w64-mingw32-ar`
- `x86_64-w64-mingw32-ranlib`
- `x86_64-w64-mingw32-windres`
- `cmake`
- `make`

### Build Steps

```sh
make
```

### Output

- `public/bin/Trading-Floor.exe`

### Internal Dependencies

The build process bundles several internal components:

- `lib/CppClient/` — IBKR TWS C++ API SDK
- `lib/protobuf/` — Protobuf and Abseil C++ support
- `lib/IntelRDFPMathLib20U4/` — precise decimal arithmetic

## Architecture

The executable is assembled from:

```
src/main.cpp         # app entry point and window initialization
src/api/gateway.h    # TradingAPI interface, IBKR connection logic
src/gui/*.h          # window modules and UI handling
src/api/shared.h     # shared helpers, registry, dark mode, process helpers
src/api/registry.h   # registry-backed settings
src/api/sound.h      # audio notification queue
```

## Usage

- Use the tray icon to show/hide the main dashboard.
- Open windows for account, positions, orders, market quotes, news, and Time & Sales.
- Manage watchlists from the Book window and reuse them in Ticker/News.
- Use the Settings window to tune app behavior.
- Open Debug Log for live diagnostics.

## Troubleshooting

- If the app cannot locate `ibgateway.exe`, it will prompt you to select the path.
- If IB Gateway is not running, enable Auto-start Gateway or start it manually.
- Use the Debug Log to inspect connection, market data, and news events.
- Delete the registry root to reset preferences:

```
Computer\HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor
```

## Uninstall

- Delete `Trading-Floor.exe`.
- Remove the registry key `Computer\HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor`.

## License & Legal

- Provided as-is for educational and personal use.
- Interactive Brokers® and IB Gateway® are trademarks of Interactive Brokers.
- Ensure compliance with IBKR API usage policies.
- Use at your own risk; verify behavior before trading live.
