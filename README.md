# 💼 IBKR Gateway: Trading Floor

A lightweight Windows tray application that connects to Interactive Brokers Gateway and surfaces live account, market, and trading data in a compact, portable Windows GUI.

> System Tray Icon: 💼

## Overview

This application is a modern desktop trading companion built on the Interactive Brokers C++ API. It is designed to work with **IBKR Gateway** (not Trader Workstation) and provides a modular, multi-window environment for professional traders.

### Core Capabilities:
- **Real-time Market Data**: Live quotes, tick-by-tick Market, and symbol watchlists with advanced filtering and customizable layouts.
- **Account Intelligence**: Live margin metrics, Net Liquidation Value with optional Text-to-Speech (TTS) alerts, and detailed P&L analytics.
- **Portfolio Management**: Comprehensive position tracking with dividend data, 52-week range analysis, and customizable tagging/color-coding.
- **Order Execution Tracking**: Real-time order status monitoring with intuitive color-coding, in-place modifications, and quick cancellation.
- **Information Hub**: Symbol-specific news headlines with provider filtering, HTML-to-RTF article preview, and a centralized watchlist manager with auto-complete.
- **User-Friendly UI**: Font zoom support for ListViews, dynamic split-panel Market, and intelligent auto-complete symbol entry in Watchlist.
- **Robust Infrastructure**: Auto-reconnecting watchdog, persistent registry-based settings, asynchronous audio notifications, and a system-tray-first design.

## Key Highlights

- **Tray-Centric Design**: Runs primarily from the system tray with dynamic status icons (Connected/Disconnected) and a context menu for quick window management.
- **Auto-Connect Watchdog**: A dedicated background timer monitors the connection to `127.0.0.1:4001` and automatically restores the session if the Gateway drops.
- **Modular Window Architecture**: A "Trading Floor" layout where 10+ independent windows (Dashboard, Coins, Diamonds, Orders, Watchlist, News, Market, Settings, Debug Log) can be arranged, persisted, and restored.
- **Advanced Watchlists**: Create and manage named symbol lists directly in the Watchlist window, which are then shared across News and Market.
- **Accessibility & Alerts**: Integrated Windows SAPI for NetLiq voice alerts and custom sound notifications for critical trading events.
- **Visual Customization**: Full Dark Mode support, list view font zooming, and high-contrast color-coding for P&L and order statuses.

## Core Modules

### 💼 Dashboard
The central command hub of the application:
- **Connection Status**: Real-time monitoring of Market Data and Trading sessions via tray tooltip and icon.
- **Quick Launch**: One-click buttons to open any module in the suite.
- **Sticky Tray Menu**: Manage window "Always On Top" states and connections directly from the tray context menu without losing focus.
- **Session Control**: Toggle between manual and automatic connection watchdog.
- **Instance Management**: Ensures a single running instance; launching a new one replaces the old process.

### 💰 Coins (Account Summary)
High-level financial health monitoring:
- **Net Liquidation Value**: Prominent display with optional **TTS audio alerts** for rapid value changes.
- **Liquidity Metrics**: Live Buying Power, Available Funds, and Excess Liquidity.
- **Margin Tracking**: Real-time Initial and Maintenance Margin requirements.
- **P&L Suite**: Daily, Unrealized, and Realized P&L with green/red color-coding.

### 📝 Orders
Precision order tracking and management:
- **Visual Status**: Color-coded rows for instant recognition:
  - 🟢 **Filled** | 🟡 **Partially Filled** | 🔵 **Submitted** | ⚪ **Cancelled/Inactive**
- **Interactive Management**: In-place modification of order price and quantity via double-click.
  - **Keyboard Controls**: Use `Enter` to confirm, `Esc` to cancel, `Tab` to switch fields, and `Up`/`Down` arrows to increment/decrement prices and quantities.
- **Quick Cancel**: Press the `Delete` or `Escape` key on a selected order in the list to immediately transmit a cancellation request.
- **Audit Trail**: Sorted by status and timestamp to prioritize the most recent updates.

### 💎 Diamonds
Deep-dive portfolio positions analysis:
- **Live Pricing**: Current bid/ask/last prices for all held instruments.
- **Performance**: Daily P&L, change %, and unrealized P&L.
- **Dividends**: Dividend yield, next dividend date, and annual dividend amounts.
- **Market Context**: 52-week range percentage and market value relative to Net Liquidation.

### 👀 Watchlist
Real-time quote monitoring with saved watchlists and fast table navigation:
- **Comprehensive Data**: Last price, bid/ask sizes, change %, dividend yield, dividend date, and annual dividend.
- **Extended Metrics**: Fundamental tick updates, 52-week ranges, and market-cap style insights.
- **Symbol Auto-Complete**: Add instruments quickly using the inline searchable symbol entry box.
- **Table Navigation**: Double-click any row to instantly open a dedicated Market window for that instrument.
- **Column Sorting**: Click headers to sort by any column (Price, Change %, Symbol, etc.). The sort preference is saved per session.
- **Registry Persistence**: All lists are stored under the Windows Registry and recovered across restarts.
- **Live Placeholder Rows**: Symbols appear immediately while market data subscriptions are still initializing.
- **Zoomable Tables**: Use `Ctrl + Mouse Wheel` on list views to resize text for readability.
- **Keyboard-Friendly Editing**: Use `Delete` key to remove lists and list items.

### 🏦 Market
High-frequency trade monitoring with split-panel filtering and symbol search:
- **Tick-by-Tick Feed**: Live trade price, size, time, and exchange for the active symbol.
- **Filtered Views**: Toggle between full trades, top 100, and top 1000 size filters.
- **Interactive Splitters**: Customize your view with **draggable vertical and horizontal splitters**. The application remembers your preferred layout for each symbol.
- **Multi-Window Support**: Open many Market sessions at once for different instruments.
- **Market Search**: A dedicated search dialog with auto-complete and keyboard navigation (`Arrows` + `Enter`) to quickly find and launch new market windows.
- **Persistent Preferences**: Filter states and splitter positions are saved to the registry and restored automatically.

### 📰 News
Contextual market intelligence with provider filtering and article preview:
- **Symbol-Specific Request**: Select a symbol from a saved list and request news directly for that contract.
- **Provider Filter**: Narrow headlines by news provider or show all available sources.
- **Persistent State**: Restores the last selected news list, last selected symbol, and provider filter across sessions.
- **Rich Article Preview**: Downloads news bodies and converts basic HTML into RTF for clean RichEdit display.
- **Binary/PDF Handling**: Detects unsupported article formats and notifies the user when inline preview is unavailable.

### ⚙️ Settings
Personalized application configuration with immediate feedback:
- **Gateway Automation**: Auto-start IBKR Gateway on launch and optionally kill it on exit.
- **Dark Mode**: Applies instantly across all windows with a full repaint for UI consistency.
- **Asynchronous Audio**: Toggle global event sounds. Audio is processed via a dedicated **async sound queue** to ensure zero impact on UI responsiveness.
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
- open market sessions
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
- Open windows for account, positions, orders, market quotes, news, and Market.
- Manage watchlists directly in the Watchlist window and reuse them in News and Market.
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
