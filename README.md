# 💰 TWS API: Trading Floor

A lightweight Windows tray application that connects to the Interactive Brokers TWS API and surfaces live account, market, and trading data in a compact, portable Windows GUI.

> **System Tray Icon:** 💰

---

## 🚀 Quick Start

### Pre-Flight Checklist
Before launching the application, ensure the following:
- [ ] **TWS or IB Gateway** is installed and running.
- [ ] **API Access** is enabled in TWS/Gateway settings (`Allow connections from localhost loopback interface`).
- [ ] **Market Data Permissions** are active on your IBKR account.

### Installation
1. Download the latest `Trading-Floor.exe`.
2. Place it in your preferred directory (e.g., `C:\Program Files\Trading-Floor\`).
3. Run `Trading-Floor.exe`.
4. The app will automatically attempt to connect to `127.0.0.1:4001` or `7496`.

> **Tip:** Enable **Auto-start IBKR Gateway** in Settings to let the app manage the gateway process for you.

---

## 📖 Overview

A modern desktop trading companion built on the Interactive Brokers C++ API. It provides a modular, multi-window environment optimized for rapid day trading and professional traders who need high-density data without the bulk of a full trading platform.

### Core Capabilities
*   **Real-time Market Data**: Live quotes, Level 2 depth, and tick-by-tick feeds.
*   **Account Intelligence**: Live margin metrics and Net Liquidation Value with SAPI Text-to-Speech (TTS) alerts.
*   **Portfolio Management**: Position tracking with dividends and custom tab grouping.
*   **Order Tracking**: Real-time status monitoring with in-place modifications and rapid cancellation.
*   **Market Intelligence**: Advanced scanner for finding trading opportunities based on multiple criteria.
*   **Robust Infrastructure**: Auto-reconnecting watchdog, registry-based persistence, and async audio notifications.

---

## 🛠 Core Modules

### 💰 Dashboard (The Command Center)
The primary tray window combining account metrics with centralized window management.
*   **Account Summary**: Live Net Liquidation Value, Daily P&L (with clickable TTS speaker icon), and detailed margin metrics (Buying Power, Maintenance Margin, Unrealized/Realized PnL, Dividends, Gross Position, Accrued Cash, EUR/USD Cash).
*   **Window Launcher**: One-click access to all other modules.
*   **Tray Integration**: Right-click the 💰 icon for window toggles, "Always-On-Top" markers **[ ★ ]**, and quick access to Settings/Logs.
*   **Session Control**: Toggle the auto-connect watchdog to ensure your session stays alive.

### 📝 Orders (Precision Execution)
Real-time order tracking with an emphasis on speed and accuracy.
*   **Visual Status**: 🟢 Filled | 🟡 Partially Filled | 🔵 Submitted | ⚪ Cancelled/Inactive.
*   **Modify Orders**: Quick-update limit prices and quantities for active orders.
*   **Rapid Action**: Press `Delete` or `Esc` on a selected order for immediate cancellation.

### 💎 Diamonds (Portfolio Analysis)
Deep-dive analysis of held positions with advanced grouping.
*   **Performance**: Daily P&L, Unrealized P&L, and unrealized % change with high-contrast color-coding.
*   **Dividends**: Track yield %, next dividend date, annual amounts, and market value.
*   **Custom Tabs**: Three filterable tabs — *Growth*, *High-Yield Dividends*, and *Quarantine* (checkboxes at bottom).
*   **Symbol Colors**: Assign one of six colors (Red, Green, Blue, Purple, Gold, Brown) per position for visual categorization.
*   **Deferred Sort**: Header-click to sort by any column; zero-flicker re-sorting via timer.

### 👀 Watchlist (Market Monitoring)
Real-time quote monitoring with cross-window synchronization.
*   **Smart Entry**: Inline symbol entry with real-time auto-complete.
*   **Named Lists**: Create multiple saved watchlists (stored in Registry) shared across Scanner and Market windows.
*   **Fast Navigation**: Double-click any symbol to instantly spawn a dedicated Market window.

### 🏦 Market (High-Frequency Data)
The core data engine for active trading. Supports infinite concurrent instances.
*   **Level 1 Data**: Real-time streaming quotes (Last, Bid, Ask, High, Low, Volume).
*   **Level 2 Depth**: Real-time bid/ask ladder in the left panel with price and size columns.
*   **Time & Sales**: Three real-time tick-by-tick streams — All, ≥100 shares, ≥1000 shares — with dynamic vertical/horizontal splitters to customize your view per symbol.
*   **Quick Order Bar**: Press `Left Ctrl` or `Right Ctrl` to reveal the rapid order entry bar (pre-filled with best bid/ask). Includes Stop-Loss and Take-Profit price fields for bracket orders.
*   **Audio Alerts**: Independent per-window TTS for price announcements.

### 🚀 Scanner (Symbol Discovery)
Advanced market scanner for finding trading opportunities based on multiple criteria.
*   **Multi-Criteria Filtering**: Combine conditions across price, volume, volatility, and fundamental metrics.
*   **Real-time Results**: Live updating scan results as market data streams in.
*   **Customizable Views**: Save and manage multiple predefined scan configurations.
*   **Quick Actions**: Double-click any result to open a dedicated Market window for that symbol.

### ⚙️ Settings & 🐞 Debug Log
*   **Settings**: Configure Dark Mode, TTS voice selection, default order quantity, Stop/Profit price defaults, Gateway path, and auto-start IBKR Gateway.
*   **Debug Log**: A live stream of API callbacks and internal events for diagnostics and troubleshooting.

---

## ⌨️ Key Shortcuts & Tips

### 🖱 Navigation & UI
| Action | Shortcut | Note |
| :--- | :--- | :--- |
| **Sort Column** | `Click Header` | Click again to reverse direction |
| **Remove Item** | `Delete` | Removes selected row/watchlist |
| **Quick Order Bar**| `L-Ctrl` / `R-Ctrl` | Toggles entry bar in Market window |

### 📝 Order Editing
| Key | Action |
| :--- | :--- |
| `Double-Click` | Enter edit mode for Price/Quantity |
| `Tab` | Cycle between Price and Quantity fields |
| `Enter` | Confirm and submit modification |
| `Esc` | Cancel order |
| `Up / Down` | Increment/Decrement values (0.01 / 1 unit) |

---

## ⚙️ Technical Details

### Registry Storage
Preferences are persisted under: `HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor`

### Build Instructions (Linux $\rightarrow$ Windows)
Cross-compile using MinGW:
```sh
# Prerequisites: x86_64-w64-mingw32-g++, cmake, make
make
# Output: public/bin/Trading-Floor.exe
```

### Architecture Highlights
- **Async Audio**: Dedicated worker thread prevents GUI freezing during sound alerts.
- **ListView Efficiency**: Uses `LVS_EX_DOUBLEBUFFER` and `NM_CUSTOMDRAW` to ensure flicker-free, high-frequency updates.
- **Registry Abstraction**: Generic helper functions eliminate boilerplate for 15+ persistent settings.

---

## 🛠 Troubleshooting
*   **No Connection?** Check if TWS/Gateway is running and "Allow connections from localhost" is checked.
*   **Wrong Path?** If the app can't find the gateway, it will prompt you to select the `.exe` path.
*   **Reset App?** Delete the registry root: `Computer\HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor`

## ⚖️ License & Legal
- Provided as-is for educational and personal use.
- Interactive Brokers® and IB Gateway® are trademarks of Interactive Brokers.
- Use at your own risk; verify behavior before trading live.

