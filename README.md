# 💰 TWS API: Trading Floor

A lightweight Windows tray application that connects to the Interactive Brokers TWS API and surfaces live account, market, and trading data in a compact, portable Windows GUI.

> System Tray Icon: 💰

## Overview

This application is a modern desktop trading companion built on the Interactive Brokers C++ API. It is designed to work with **TWS** or **IB Gateway** and provides a modular, multi-window environment for professional traders.

### Core Capabilities:
- **Real-time Market Data**: Live quotes, tick-by-tick Market, and symbol watchlists with advanced filtering and customizable layouts.
- **Account Intelligence**: Live margin metrics, Net Liquidation Value with optional Text-to-Speech (TTS) alerts, and detailed P&L analytics.
- **Portfolio Management**: Comprehensive position tracking with dividend data, 52-week range analysis, and customizable tagging/color-coding.
- **Order Execution Tracking**: Real-time order status monitoring with intuitive color-coding, in-place modifications, and quick cancellation.
- **Information Hub**: Symbol-specific news headlines with provider filtering, HTML-to-RTF article preview, and a centralized watchlist manager with auto-complete.
- **User-Friendly UI**: Font zoom support for ListViews, dynamic split-panel Market, and intelligent auto-complete symbol entry in Watchlist.
- **Rapid Trading**: Integrated quick-order entry bars in Market windows for high-speed execution.
- **Robust Infrastructure**: Auto-reconnecting watchdog, persistent registry-based settings, asynchronous audio notifications, and a system-tray-first design.

## Key Highlights

- **Tray-Centric Design**: Runs primarily from the system tray with dynamic status icons (Connected/Disconnected) and a context menu for quick window management.
- **Auto-Connect Watchdog**: A dedicated background timer monitors the connection to `127.0.0.1:4001` or `127.0.0.1:7496` and automatically restores the session if the Gateway drops.
- **Modular Window Architecture**: A "Trading Floor" layout where 8 independent windows (Dashboard with integrated Coins, Diamonds, Orders, Watchlist, News, Market, Settings, Debug Log) can be arranged, persisted, and restored.
- **Advanced Watchlists**: Create and manage named symbol lists directly in the Watchlist window, which are then shared across News and Market.
- **Accessibility & Alerts**: Integrated Windows SAPI for NetLiq voice alerts and custom sound notifications for critical trading events.
- **Visual Customization**: Full Dark Mode support, list view font zooming, and high-contrast color-coding for P&L and order statuses.

## Core Modules

### 💰 Dashboard (Account Summary & Window Launcher)
The primary tray window combining account metrics with centralized window management:

**Account Summary Display:**
- **Net Liquidation Value**: Prominent display with live currency detection (EUR/USD/Multi-currency).
- **Daily P&L with TTS**: Large, color-coded daily profit/loss with optional **clickable speaker icon** for voice announcements using Windows SAPI text-to-speech. Automatically speaks on account updates (configurable voice).
- **Daily P&L %**: Percentage return relative to Net Liquidation Value.
- **Account Metrics Table**: Comprehensive live data including:
  - Unrealized & Realized P&L (green/red color-coded)
  - Dividends accrued
  - Gross Position Value
  - Buying Power & Maintenance Margin
  - Cash Balances (Total, EUR, USD) with automatic currency suffix

**Window Launcher & Tray Features:**
- **Connection Status**: Real-time monitoring of Market Data and Trading sessions via tray tooltip and icon. Icon changes color based on connection state.
- **Quick Launch Buttons**: One-click access to Orders, Diamonds, Watchlist, Market, News, and Settings windows.
- **Always-On-Top Indicator**: Tray context menu shows **[ ★ ]** marker for windows pinned above others, making it easy to see which are topmost.
- **Sticky Tray Menu**: Right-click the tray icon to open a context menu with:
  - Window toggles (show/hide all windows without losing focus)
  - Always On Top mode for each window (toggle with **[ ★ ]** indicator)
  - Quick access to Settings and Debug Log
- **Session Control**: Toggle between manual and automatic connection watchdog. Auto-reconnect monitors the connection to `127.0.0.1:4001` and automatically restores the session if the Gateway drops.
- **Instance Management**: Ensures a single running instance; launching a new one replaces the old process.

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
High-frequency trade monitoring with Level 2 depth, portfolio snapshot, and dynamic splitter panels:

**Header Metrics:**
- **Left Controls**: Speaker icon for per-window TTS + filter checkbox for trade size tiers (full/top 100/top 1000).
- **OHLC Stats**: Open, Close, High, Low prices displayed in the header row 1.
- **Portfolio Snapshot**: Position size and average purchase price (Pos / Avg) in header row 2, updated in real-time.
- **Quote Block**: Large Last Price with change amount and percentage (color-coded green/red). Current Bid/Ask with sizes displayed on the right.
- **PnL Display**: Daily and Unrealized P&L shown in the header when available (account position data).

**Level 2 Depth & Tick Feed:**
- **Level 2 Depth Book** (left panel): Real-time bid/ask ladder with size at each price level for instant market structure visibility. Fixed width panel with bid prices, sizes, ask prices, and sizes in a four-column layout.
- **Tick-by-Tick Feed** (right panel): Live trade price, size, time, and exchange for the active symbol. Sortable by price, size, and time.
- **Filtered Views**: Toggle between full trades, top 100, and top 1000 size filters to focus on significant activity via the header filter checkbox.

**Interactive Layout:**
- **Draggable Splitters**: Customize your view with **vertical and horizontal splitters** separating Level 2 (left) from Tick-by-Tick (right) and header from data. Layout is saved per symbol to the registry.
- **Font Zoom**: Use `Ctrl + Mouse Wheel` to zoom the Tick-by-Tick list for readability. Zoom level persists per Market window.

**Order Entry & TTS:**
- **Quick Order Entry**: Press `Left Ctrl` or `Right Ctrl` to reveal a rapid order entry bar, pre-filled with the current best bid/ask and your configured default quantity (set in Settings). Entry bar shows the selected side (BUY/SELL) for clarity.
- **Per-Window TTS**: Optional voice announcements of the **Last Price** controllable via a **speaker icon** in the header. Each Market window maintains independent TTS state (21-second update intervals).

**Multi-Window & Search:**
- **Multi-Window Support**: Open many Market sessions at once for different instruments with independent splitter layouts, TTS states, and filter preferences.
- **Market Search**: A dedicated search dialog with auto-complete and keyboard navigation (`Arrows` + `Enter`) to quickly find and launch new market windows.
- **Persistent Preferences**: Filter states, splitter positions, zoom levels, and TTS state are saved per symbol to the registry and restored automatically.

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
- **Gateway Automation**: Auto-start TWS or IB Gateway on launch and optionally kill it on exit. Prompts for gateway executable path if not found.
- **Dark Mode**: Applies instantly across all windows with a full repaint for UI consistency.
- **Asynchronous Audio**: Toggle global event sounds. Audio is processed via a dedicated **async sound queue** to ensure zero impact on UI responsiveness.
- **Default Order Quantity**: Configure the default quantity pre-filled in rapid order entry bars across all Market windows.
- **TTS Voice Selection**: Choose from all available system voices for announcements and dashboard P&L alerts. Voices are enumerated from Windows SAPI with smart defaults.
- **Debug Log Access**: Open the live debug console directly from Settings for real-time diagnostics.
- **Registry Backed**: All settings (Dark Mode, sound toggle, TTS voice, auto-gateway, kill-on-exit, default quantity) are persisted to the Windows Registry.

### 🐞 Debug Log
Runtime diagnostics and transparency for deeper troubleshooting:
- **Live Event Stream**: Captures real-time API callbacks, connection state, and internal debug messages.
- **Buffered Flush**: Preserves messages generated before the window opens and displays them on launch.
- **Topmost Window**: The debug log can be displayed as an always-on-top diagnostics pane.
- **Developer Insights**: Ideal for tracking connection failures, symbol search results, and news requests.

## Requirements

### Software

- Windows 10 or 11 (x86_64)
- Trader Workstation or IBKR Gateway
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

- **Settings/**: Dark Mode, Sound toggle, TTS voice selection, Auto-gateway, Kill-on-exit flags, Default order quantity, Gateway executable path
- **Per-Window Keys** (Dashboard, Orders, Diamonds, Market, Watchlist, News, Settings, Debug Log): Window position/size, sort column, always-on-top state, zoom level
- **Watchlist/**: Open watchlist names and members
- **Diamonds/**: Tab assignments (Growth, Dividends, Quarantine), custom symbol colors, tab visibility state
- **Market/**: Per-symbol splitter positions, filter state, session list, per-window TTS state
- **News/**: Last selected watchlist, symbol, provider filter
- **Zoom Settings**: Font size per window (Zoom_Market, Zoom_Diamonds, Zoom_Orders, Zoom_Watchlist, Zoom_News)

The executable is portable and can be placed anywhere on disk. There is no required installation folder. `Trading-Floor.exe` may be run from any directory.

## Performance & Optimization

- **UI Responsiveness**: Audio notifications run on a dedicated worker thread; zero blocking on the main GUI thread.
- **ListView Efficiency**: 
  - `LVS_EX_DOUBLEBUFFER` prevents flicker during rapid updates in Tick-by-Tick, Level 2, Orders, Diamonds, and Watchlist.
  - Custom draw with `NM_CUSTOMDRAW` eliminates per-row allocation overhead.
  - Batch updates (suspend redraw, insert/update all items, resume redraw) for bulk operations.
- **Memory Management**: Smart use of C++ move semantics, minimal allocations in hot paths (tick handling, market data updates, order updates).
- **Registry Caching**: Settings are read once at startup; frequent lookups use in-memory caches.
- **Market Data Optimization**: 
  - Level 2 depth and tick-by-tick data are deduplicated to prevent UI saturation.
  - Per-symbol splitter and filter state caching reduces registry lookups on each Market window.
  - TTS updates use 21-second intervals to avoid excessive voice synthesis.

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
src/gui/dashboard.h       # Main tray window + integrated Coins + TTS voice alerts
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

- Use the tray icon to show/hide the main Dashboard (which displays live account metrics, Net Liquidation, Daily P&L, and quick-launch buttons).
- Open **Market** windows for each symbol to monitor Level 2 depth, tick-by-tick trades, position snapshot (size & avg price), and P&L metrics.
- Open **Diamonds** for portfolio-wide positions analysis with dividends, P&L, and color-coded grouping.
- Open **Orders** for real-time order tracking with in-place editing and quick cancellation.
- Open **Watchlist** to create and manage named symbol lists, then reuse them in News and Market windows.
- Open **News** to fetch and read headlines for symbols from your watchlists.
- Use the **Settings** window to tune app behavior, select TTS voice, configure default order quantity, and manage Gateway automation.
- Open **Debug Log** for live diagnostics of API callbacks and internal events.

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
- **Drag Splitters**: Resize the Level 2 depth panel (left) and tick data (right) by dragging the vertical divider. Resize header area by dragging the horizontal splitter. Layout is saved per symbol.
- **Ctrl + Mouse Wheel**: Zoom the Tick-by-Tick list in/out for readability. Zoom level persists per Market window.
- **Filter Checkbox** (header, left side): Toggle between full trades, top 100, and top 1000 size tiers.
- **Click Speaker Icon** (header, left side): Announce current Last Price via text-to-speech. Each Market window has independent TTS control (21-second update intervals).

### General
- **Right-Click** on tray icon: Open context menu with window toggles and Always On Top controls (marked with **[ ★ ]** when active).
- **Click Speaker Icon** on Dashboard: Announce current Daily P&L and account summary via text-to-speech.
- **Left Ctrl / Right Ctrl**: Press in any Market window to toggle rapid order entry bar (pre-filled with current best bid/ask and default quantity).

## Troubleshooting

- If the app cannot locate `tws.exe` or `ibgateway.exe`, it will prompt you to select the path.
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
