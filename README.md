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
- **Connection Status**: Real-time monitoring of Market Data and Trading sessions via tray tooltip and icon. Icon changes color based on connection state.
- **Quick Launch**: One-click buttons to open any module in the suite (Coins, Orders, Watchlist, Market, News, Diamonds, Settings).
- **Sticky Tray Menu**: Right-click the tray icon to open a context menu with:
  - Window toggles (show/hide all windows without losing focus)
  - Always On Top mode for each window (pin windows above others)
  - Quick access to Settings and Debug Log
- **Session Control**: Toggle between manual and automatic connection watchdog. Auto-reconnect monitors the connection to `127.0.0.1:4001` and automatically restores the session if the Gateway drops.
- **Instance Management**: Ensures a single running instance; launching a new one replaces the old process.

### 💰 Coins (Account Summary)
High-level financial health monitoring with voice alerts:
- **Net Liquidation Value**: Prominent display with optional **clickable speaker icon for voice announcements** of rapid value changes. Uses the voice selected in Settings.
- **Liquidity Metrics**: Live Buying Power, Available Funds, and Excess Liquidity with color-coded formatting.
- **Margin Tracking**: Real-time Initial and Maintenance Margin requirements.
- **P&L Suite**: Daily, Unrealized, and Realized P&L with green/red color-coding for instant recognition.
- **TTS Alerts**: On account summary updates, automatically announce key metrics using Windows SAPI text-to-speech if enabled.

### 📝 Orders
Precision order tracking and management with live updates:
- **Visual Status**: Color-coded rows for instant recognition:
  - 🟢 **Filled** | 🟡 **Partially Filled** | 🔵 **Submitted** | ⚪ **Cancelled/Inactive**
- **Interactive Management**: In-place modification of order price and quantity via double-click.
  - **Keyboard Controls**: Use `Enter` to confirm, `Esc` to cancel, `Tab` to switch fields between Price and Quantity, `Up`/`Down` arrows to increment/decrement values, and `Ctrl+Z` to undo changes before submission.
- **Quick Cancel**: Press the `Delete` or `Escape` key on a selected order in the list to immediately transmit a cancellation request.
- **Column Sorting**: Click column headers to sort by Price, Quantity, Status, Timestamp, etc. Sort indicators show current direction.
- **Audit Trail**: Sorted by status and timestamp to prioritize the most recent updates.
- **Number Formatting**: All prices and quantities display with comma separators for easy reading at a glance.

### 💎 Diamonds
Deep-dive portfolio positions analysis with grouping and color-coding:
- **Live Pricing**: Current bid/ask/last prices for all held instruments with real-time updates.
- **Performance Metrics**: Daily P&L, change %, and unrealized P&L with color-coded rows (green = profit, red = loss).
- **Dividends**: Dividend yield %, next dividend date, and annual dividend amounts.
- **Market Context**: 52-week range percentage and market value relative to Net Liquidation.
- **Tab Filtering**: Group positions into tabs (Growth, High-Yield Dividends, Quarantine) for focused analysis. Tabs persist to registry.
- **Color-Coding**: Assign custom colors (Red, Green, Blue, Purple, Gold, Brown) to individual symbols for visual organization.
- **Quick Launch**: Double-click any position to open a dedicated Market window for that symbol.
- **Column Sorting**: Click headers to sort by any metric (Yield %, Dividend Date, P&L, etc.).

### 👀 Watchlist
Real-time quote monitoring with saved watchlists, fast table navigation, and cross-window syncing:
- **Comprehensive Data**: Last price, bid/ask sizes, change %, dividend yield, dividend date, and annual dividend per instrument.
- **Extended Metrics**: Fundamental tick updates, 52-week high/low ranges, market capitalization, and performance context.
- **Symbol Auto-Complete**: Add instruments quickly using the inline searchable symbol entry box with real-time suggestions.
- **Table Navigation**: Double-click any row to instantly open a dedicated Market window for that instrument.
- **Column Sorting**: Click headers to sort by any column (Price, Change %, Yield, Symbol, etc.). Sort preference is saved per session.
- **Named Watchlists**: Create, save, and manage multiple named symbol lists directly in the window. Lists are stored under the Windows Registry and recovered across restarts.
- **Cross-Window Sharing**: All watchlists created here are automatically available in News and Market windows for symbol selection.
- **Live Placeholder Rows**: Symbols appear immediately in the list while market data subscriptions are still initializing.
- **Zoomable Tables**: Use `Ctrl + Mouse Wheel` on the list view to resize text for readability.
- **Keyboard-Friendly Editing**: Press `Delete` to remove selected watchlists or individual list items.

### 🏦 Market
High-frequency trade monitoring with Level 2 depth and dynamic splitter panels:
- **Level 2 Depth Book**: Real-time bid/ask ladder with size at each price level on the left panel for instant market structure visibility.
- **Tick-by-Tick Feed**: Live trade price, size, time, and exchange for the active symbol on the right side.
- **Filtered Views**: Toggle between full trades, top 100, and top 1000 size filters to focus on significant activity.
- **Interactive Splitters**: Customize your view with **draggable vertical and horizontal splitters**. The application remembers your preferred layout for each symbol.
- **Multi-Window Support**: Open many Market sessions at once for different instruments.
- **Market Search**: A dedicated search dialog with auto-complete and keyboard navigation (`Arrows` + `Enter`) to quickly find and launch new market windows.
- **Persistent Preferences**: Filter states, splitter positions, and sort preferences are saved to the registry and restored automatically.
- **Visual Indicators**: Sort column headers display arrows (↑↓) to show current sort direction.

### 📰 News
Contextual market intelligence with provider filtering and rich article preview:
- **Symbol-Specific Request**: Select a symbol from a saved watchlist and request news directly for that contract.
- **Provider Filter**: Narrow headlines by news provider (Bloomberg, Reuters, etc.) or show all available sources.
- **Persistent State**: Restores the last selected news watchlist, symbol, and provider filter across sessions via registry.
- **Rich Article Preview**: Downloads and converts HTML news bodies into RTF format for clean display in RichEdit controls.
- **Binary/PDF Handling**: Intelligently detects unsupported article formats (PDF, images) and notifies the user when inline preview is unavailable.
- **Multiple Watchlists**: Access any watchlist created in the Watchlist window for fast symbol switching.
- **Headline Metadata**: View headline, timestamp, provider code, and article ID for each story.

### ⚙️ Settings
Personalized application configuration with immediate feedback:
- **Gateway Automation**: Auto-start IBKR Gateway on launch and optionally kill it on exit.
- **Dark Mode**: Applies instantly across all windows with a full repaint for UI consistency.
- **Asynchronous Audio**: Toggle global event sounds. Audio is processed via a dedicated **async sound queue** to ensure zero impact on UI responsiveness.
- **TTS Voice Selection**: Choose from all available system voices for announcements. Voices are enumerated from Windows SAPI, with a smart default (Catalan/Herena if available).
- **Debug Log Access**: Open the live debug console directly from Settings.
- **Registry Backed**: Saves Dark Mode, sound, voice preference, auto-gateway, and kill-on-exit settings to the registry.

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
2. Place it anywhere on your machine, for example `C:\Program Files\Trading-Floor\`.
3. Run `Trading-Floor.exe`.
4. Start and authenticate IBKR Gateway.
5. The app will auto-connect within seconds when Gateway is available.

> Tip: enable **Auto-start IBKR Gateway** in Settings to have the app launch Gateway automatically if it is not already running.

## Registry Storage

All application state and preferences are stored under:

```
HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor
```

**Persisted settings include:**

- **Settings/**: Dark Mode, Sound toggle, TTS voice selection, Auto-gateway, Kill-gateway flags, Gateway path
- **Per-Window Keys** (Dashboard, Orders, Coins, etc.): Window position/size, sort column, always-on-top state
- **Watchlist/**: Open watchlist names and members
- **Diamonds/**: Tab assignments (Growth, Dividends, Quarantine), custom symbol colors
- **Market/**: Per-symbol splitter positions, filter state, session list
- **News/**: Last selected watchlist, symbol, provider filter
- **Zoom Settings**: Font size per window (Zoom_Diamonds, Zoom_Orders, etc.)

The executable is portable and can be placed anywhere on disk. There is no required installation folder. `Trading-Floor.exe` may be run from any directory.

## Performance & Optimization

- **UI Responsiveness**: Audio notifications run on a dedicated worker thread; zero blocking on the main GUI thread.
- **ListView Efficiency**: 
  - `LVS_EX_DOUBLEBUFFER` prevents flicker during rapid updates.
  - Custom draw with `NM_CUSTOMDRAW` eliminates per-row allocation overhead.
  - Batch updates (suspend redraw, insert/update all items, resume redraw) for bulk operations.
- **Memory Management**: Smart use of C++ move semantics, minimal allocations in hot paths (tick handling, market data updates).
- **Registry Caching**: Settings are read once at startup; frequent lookups use in-memory caches.
- **Market Data Optimization**: Level 2 and tick-by-tick data are deduplicated and rate-limited to prevent UI saturation.

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

The application uses a modular header-based architecture with a single `.exe` and no external DLL dependencies (except Windows system libraries).

### Code Organization

```
src/main.cpp              # Entry point, tray icon setup, main message loop
src/api/gateway.h         # TradingAPI class: IBKR connection logic & data models
src/api/registry.h        # Registry helpers: generic DWORD/string get/set/delete
src/api/shared.h          # Shared UI components: dark mode, fonts, layout, dialogs
src/api/sound.h           # Async sound queue for notifications
src/gui/dashboard.h       # Main tray window + TTS voice alerts
src/gui/coins.h           # Account summary with margin/P&L
src/gui/orders.h          # Order tracking with in-place editing
src/gui/diamonds.h        # Portfolio with dividends, grouping, colors
src/gui/watchlist.h       # Named symbol lists with auto-complete
src/gui/market.h          # Level 2 depth + tick-by-tick with splitters
src/gui/news.h            # News headlines with RTF preview
src/gui/settings.h        # Preferences UI with voice selector
```

### Key Design Patterns

- **Registry Abstraction**: `RegSetString()`, `RegGetString()`, `RegSetDword()`, `RegGetDword()` eliminate boilerplate across 15+ settings functions.
- **Shared UI Helpers**: Common ListView, checkbox, sort, and custom-draw logic reduces duplication across 7 GUI windows.
- **Async Audio**: Dedicated worker thread processes sound notifications without blocking UI.
- **Window Subclassing**: Standardized `SetWindowSubclass` approach for font zoom, edit validation, and search auto-complete.
- **Event-Driven Updates**: Windows register as observers via `api.setDiamondsWindow()`, `api.setOrdersWindow()`, etc., and receive `WM_API_UPDATE` and `WM_*_UPDATE` messages.

### Dependencies

- **libcppClient** (Interactive Brokers C++ API): TWS/Gateway connection & data feed
- **libprotobuf & abseil**: Serialization for IBKR's internal protocol
- **Windows SAPI**: Text-to-speech voice synthesis
- **MinGW GCC**: Cross-compiled from Linux for Windows x86_64

## Usage

- Use the tray icon to show/hide the main dashboard.
- Open windows for account, positions, orders, market quotes, news, and Market.
- Manage watchlists directly in the Watchlist window and reuse them in News and Market.
- Use the Settings window to tune app behavior.
- Open Debug Log for live diagnostics.

## Keyboard Shortcuts & Tips

### List View Navigation
- **Ctrl + Mouse Wheel**: Zoom text in/out on any list view (applies per window and persists to registry).
- **Click Column Header**: Sort by that column; click again to reverse direction.
- **Delete**: Remove selected rows or lists (context-dependent: removes watchlist entries, deletes orders, etc.).

### Order Editing (Orders Window)
- **Double-Click** on a row to enter edit mode for Price or Quantity.
- **Tab**: Move to the next editable field (cycles between Price and Quantity).
- **Enter**: Confirm modifications and submit the order change.
- **Esc**: Cancel edits without submitting.
- **Up/Down Arrows**: Increment/decrement the current value in 0.01 or 1 unit steps.

### Market Window
- **Arrows + Enter**: In Market Search dialog, navigate symbol list and press Enter to open.
- **Drag Splitters**: Resize the Level 2 depth panel (left) and tick data (right) by dragging the vertical divider.

### General
- **Right-Click** on tray icon: Open context menu with window toggles and Always On Top controls.
- **Click Speaker Icon** (on Coins window): Announce current account metrics via text-to-speech.

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
