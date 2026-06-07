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

> **Pro Tip:** Enable **Auto-start IBKR Gateway** in Settings to let the app manage the gateway process for you.

---

## 📖 Overview

A modern desktop trading companion built on the Interactive Brokers C++ API. It provides a modular, multi-window environment optimized for rapid day trading and professional traders who need high-density data without the bulk of a full trading platform.

### Core Capabilities
*   **Real-time Market Data**: Live quotes, Level 2 depth, and tick-by-tick feeds.
*   **Account Intelligence**: Live margin metrics and Net Liquidation Value with SAPI Text-to-Speech (TTS) alerts.
*   **Portfolio Management**: Position tracking with dividends and 52-week range analysis.
*   **Order Tracking**: Real-time status monitoring with in-place modifications and rapid cancellation.
*   **Information Hub**: Symbol-specific news with provider filtering and RTF article previews.
*   **Robust Infrastructure**: Auto-reconnecting watchdog, registry-based persistence, and async audio notifications.

---

## 🛠 Core Modules

### 💰 Dashboard (The Command Center)
The primary tray window combining account metrics with centralized window management.
*   **Account Summary**: Live Net Liquidation Value, Daily P&L (with clickable TTS speaker icon), and detailed margin metrics (Buying Power, Maintenance Margin, etc.).
*   **Window Launcher**: One-click access to all other modules.
*   **Tray Integration**: Right-click the 💰 icon for window toggles, "Always-On-Top" markers **[ ★ ]**, and quick access to Settings/Logs.
*   **Session Control**: Toggle the auto-connect watchdog to ensure your session stays alive.

### 📝 Orders (Precision Execution)
Real-time order tracking with an emphasis on speed and accuracy.
*   **Visual Status**: 🟢 Filled | 🟡 Partially Filled | 🔵 Submitted | ⚪ Cancelled/Inactive.
*   **In-Place Editing**: Double-click Price/Quantity $\rightarrow$ use `Tab` to switch $\rightarrow$ `Enter` to submit.
*   **Rapid Action**: Press `Delete` or `Esc` on a selected order for immediate cancellation.

### 💎 Diamonds (Portfolio Analysis)
Deep-dive analysis of held positions with advanced grouping.
*   **Performance**: Daily P&L and Unrealized P&L with high-contrast color-coding.
*   **Dividends**: Track yield %, next dividend date, and annual amounts.
*   **Organization**: Use custom tabs (e.g., *Growth*, *High-Yield*) and symbol colors for visual categorization.

### 👀 Watchlist (Market Monitoring)
Real-time quote monitoring with cross-window synchronization.
*   **Smart Entry**: Inline symbol entry with real-time auto-complete.
*   **Named Lists**: Create multiple saved watchlists (stored in Registry) shared across News and Market windows.
*   **Fast Navigation**: Double-click any symbol to instantly spawn a dedicated Market window.

### 🏦 Market (High-Frequency Data)
The core data engine for active trading.
*   **Level 2 Depth**: Real-time bid/ask ladder (left panel).
*   **Tick-by-Tick Feed**: Live trade stream with size and exchange filters (right panel).
*   **Dynamic Layout**: Vertical and horizontal draggable splitters to customize your view per symbol.
*   **Quick Trade**: Press `Left Ctrl` or `Right Ctrl` to reveal the rapid order entry bar (pre-filled with best bid/ask).
*   **Audio Alerts**: Independent per-window TTS for price announcements.

### 📰 News (Intelligence Hub)
Contextual news delivery based on your watchlists.
*   **Smart Filtering**: Narrow headlines by specific providers (Bloomberg, Reuters, etc.).
*   **Rich Preview**: Converts HTML news bodies into clean RTF for a distraction-free reading experience.

### ⚙️ Settings & 🐞 Debug Log
*   **Settings**: Configure Dark Mode, TTS voice selection, default order quantity, and Gateway automation.
*   **Debug Log**: A live stream of API callbacks and internal events for diagnostics and troubleshooting.

---

## ⌨️ Key Shortcuts & Tips

### 🖱 Navigation & UI
| Action | Shortcut | Note |
| :--- | :--- | :--- |
| **Zoom Text** | `Ctrl` + `Mouse Wheel` | Persists per window in Registry |
| **Sort Column** | `Click Header` | Click again to reverse direction |
| **Remove Item** | `Delete` | Removes selected row/watchlist |
| **Quick Order Bar**| `L-Ctrl` / `R-Ctrl` | Toggles entry bar in Market window |

### 📝 Order Editing
| Key | Action |
| :--- | :--- |
| `Double-Click` | Enter edit mode for Price/Quantity |
| `Tab` | Cycle between Price and Quantity fields |
| `Enter` | Confirm and submit modification |
| `Esc` | Cancel edits |
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
