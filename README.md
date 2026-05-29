# 💼 IBKR Gateway: Trading Floor

A lightweight Windows tray application that connects to Interactive Brokers Gateway and surfaces live account, market, and trading data in a compact, portable Windows GUI.

> System Tray Icon: 💼

## Overview

This application is a modern desktop trading companion built on the Interactive Brokers C++ API. It is designed to work with **IBKR Gateway** (not Trader Workstation) and provides a modular, multi-window environment for professional traders.

### Core Capabilities:
- **Real-time Market Data**: Live quotes, tick-by-tick Time & Sales, and symbol watchlists.
- **Account Intelligence**: Live margin metrics, Net Liquidation Value with optional Text-to-Speech (TTS) alerts, and detailed P&L analytics.
- **Portfolio Management**: Comprehensive position tracking with dividend data and 52-week range analysis.
- **Order Execution Tracking**: Real-time order status monitoring with intuitive color-coding and in-place modifications.
- **Information Hub**: Symbol-specific news headlines with provider filtering, HTML-to-RTF article preview, and a centralized watchlist manager (Book).
- **User-Friendly UI**: Font zoom support for ListViews, split-panel Time & Sales, and auto-complete symbol entry in Book.
- **Robust Infrastructure**: Auto-reconnecting watchdog, persistent registry-based settings, and a system-tray-first design.

## Key Highlights

- **Tray-Centric Design**: Runs primarily from the system tray with dynamic status icons (Connected/Disconnected) and a context menu for quick window management.
- **Auto-Connect Watchdog**: A dedicated background timer monitors the connection to `127.0.0.1:4001` and automatically restores the session if the Gateway drops.
- **Modular Window Architecture**: A "Trading Floor" layout where 10+ independent windows (Dashboard, Coins, Diamonds, Orders, Watchlist, News, Time & Sales, Book, Settings, Debug Log) can be arranged, persisted, and restored.
- **Advanced Watchlists**: Create and manage named symbol lists in the **Book** module, which are then shared across the Watchlist, News, and Time & Sales windows.
- **Accessibility & Alerts**: Integrated Windows SAPI for NetLiq voice alerts and custom sound notifications for critical trading events.
- **Visual Customization**: Full Dark Mode support, list view font zooming, and high-contrast color-coding for P&L and order statuses.

## Core Modules

### 🖥️ Dashboard
The central command hub of the application:
- **Connection Status**: Real-time monitoring of Market Data and Trading sessions.
- **Quick Launch**: One-click buttons to open any module in the suite.
- **Window Management**: Dynamic menu to toggle "Always On Top" for active Time & Sales windows.
- **Session Control**: Manual Connect/Disconnect overrides.

### 💰 Coins (Account Summary)
High-level financial health monitoring:
- **Net Liquidation Value**: Prominent display with optional **TTS audio alerts** for rapid value changes.
- **Liquidity Metrics**: Live Buying Power, Available Funds, and Excess Liquidity.
- **Margin Tracking**: Real-time Initial and Maintenance Margin requirements.
- **P&L Suite**: Daily, Unrealized, and Realized P&L with green/red color-coding.

### 💎 Diamonds (Portfolio)
Deep-dive position analysis:
- **Live Pricing**: Current bid/ask/last prices for all held instruments.
- **Performance**: Daily P&L, change %, and unrealized P&L.
- **Dividends**: Dividend yield, next dividend date, and annual dividend amounts.
- **Market Context**: 52-week range percentage and market value relative to Net Liquidation.

### 📝 Orders
Precision order tracking and management:
- **Visual Status**: Color-coded rows for instant recognition:
  - 🟢 **Filled** | 🟡 **Partially Filled** | 🔵 **Submitted** | ⚪ **Cancelled/Inactive**
- **Interactive Management**: In-place modification of order price and quantity.
- **Audit Trail**: Sorted by status and timestamp to prioritize the most recent updates.

### 📈 Watchlist
Real-time quote monitoring with saved watchlists and fast table navigation:
- **Comprehensive Data**: Last price, bid/ask sizes, change %, dividend yield, dividend date, and annual dividend.
- **Extended Metrics**: Fundamental tick updates, 52-week ranges, and market-cap style insights.
- **Saved Watchlists**: Select named symbol sets from the Book and reuse them instantly.
- **Live Placeholder Rows**: Symbols appear immediately while market data subscriptions are still initializing.
- **Zoomable Tables**: Use `Ctrl + Mouse Wheel` on list views to resize text for readability.

### 📖 Book (Symbol Manager)
The database for your reusable trading symbols:
- **CRUD Operations**: Create, rename, and delete named watchlists.
- **Symbol Auto-Complete**: Add instruments quickly using the inline searchable symbol entry box.
- **Keyboard-Friendly Editing**: Use Delete to remove list items, and Up/Down buttons to reorder symbols.
- **Registry Persistence**: All lists are stored under the Windows Registry and recovered across restarts.
- **Shared Workflow**: Feeds the Watchlist, News, and Time & Sales windows with the same saved symbol lists.

### 📰 News
Contextual market intelligence with provider filtering and article preview:
- **Symbol-Specific Request**: Select a symbol from a saved list and request news directly for that contract.
- **Provider Filter**: Narrow headlines by news provider or show all available sources.
- **Persistent State**: Restores the last selected news list, last selected symbol, and provider filter across sessions.
- **Rich Article Preview**: Downloads news bodies and converts basic HTML into RTF for clean RichEdit display.
- **Binary/PDF Handling**: Detects unsupported article formats and notifies the user when inline preview is unavailable.

### ⏱️ Time & Sales
High-frequency trade monitoring with split-panel filtering and symbol search:
- **Tick-by-Tick Feed**: Live trade price, size, time, and exchange for the active symbol.
- **Filtered Views**: Toggle between full trades, top 100, and top 1000 size filters.
- **Split Panel Layout**: Supports multiple synchronized panes for detailed trade inspection.
- **Multi-Window Support**: Open many Time & Sales sessions at once for different instruments.
- **Symbol Search Popup**: Rapidly look up symbols and open a new Time & Sales window from the search dialog.

### ⚙️ Settings
Personalized application configuration with immediate feedback:
- **Gateway Automation**: Auto-start IBKR Gateway on launch and optionally kill it on exit.
- **Dark Mode**: Applies instantly across all windows with a full repaint for UI consistency.
- **Sound Notifications**: Enable or disable global event sounds from one place.
- **Debug Log Access**: Open the live debug console directly from Settings.
- **Registry Backed**: Saves Dark Mode, sound, auto-gateway, and kill-on-exit preferences to the registry.

### 🐞 Debug Log
Runtime diagnostics and transparency for deeper troubleshooting:
- **Live Event Stream**: Captures real-time API callbacks, connection state, and internal debug messages.
- **Buffered Flush**: Preserves messages generated before the window opens and displays them on launch.
- **Topmost Window**: The debug log can be displayed as an always-on-top diagnostics pane.
- **Developer Insights**: Ideal for tracking connection failures, symbol search results, and news requests.

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

The application does not create any configuration files in the filesystem; it stores its settings only in the Windows Registry.

The executable is portable and can be placed anywhere on disk. There is no required installation folder. `Trading-Floor.exe` may be run from any directory.

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

- `lib/CppClient/`: IBKR TWS C++ API SDK
- `lib/protobuf/`: Protobuf and Abseil C++ support
- `lib/IntelRDFPMathLib20U4/`: precise decimal arithmetic

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
- Manage watchlists from the Book window and reuse them in Watchlist/News.
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
