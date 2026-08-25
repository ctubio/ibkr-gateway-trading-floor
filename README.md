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
4. The app will automatically attempt to connect to `127.0.0.1:4001` or `127.0.0.1:7496`.

> **Tip:** Enable **Auto-start IBKR Gateway** in Settings to let the app manage the gateway process for you.

---

## 📖 Overview

A modern desktop trading companion built on the Interactive Brokers C++ API. It provides a modular, multi-window environment optimized for rapid day trading and professional traders who need high-density data without the bulk of a full trading platform.

### Core Capabilities
*   **Real-time Market Data**: Live quotes, Level 2 depth, and tick-by-tick feeds.
*   **Account Intelligence**: Live margin metrics and Net Liquidation Value with SAPI Text-to-Speech (TTS) alerts.
*   **Portfolio Management**: Position tracking with dividends and custom tab grouping.
*   **Order Tracking**: Real-time status monitoring with in-place modifications and rapid cancellation.
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

### 🏦 Market (High-Frequency Data)
The core data engine for active trading. Supports infinite concurrent instances.
*   **Level 1 Data**: Real-time streaming quotes (Last, Bid, Ask, High, Low, Volume).
*   **Level 2 Depth**: Real-time bid/ask ladder in the left panel with price and size columns.
*   **Time & Sales**: Three real-time tick-by-tick streams — All, ≥100 shares, ≥1000 shares — with dynamic vertical/horizontal splitters to customize your view per symbol.
*   **Quick Order Bar**: Press `Left Ctrl` or `Right Ctrl` to reveal the rapid order entry bar (pre-filled with best bid/ask). Includes Stop-Loss and Take-Profit price fields for bracket orders.
*   **Audio Alerts**: Independent per-window TTS for price announcements.

### ⚙️ Settings & 🐞 Debug Log
*   **Settings**: Configure Dark Mode, TTS voice selection, default order quantity, Stop/Profit price defaults, Gateway path, and auto-start IBKR Gateway.
*   **Debug Log**: A live stream of API callbacks and internal events for diagnostics and troubleshooting.

---

## ⌨️ Key Shortcuts & Tips

### 🖱 Global
| Key | Action |
| :--- | :--- |
| `Scroll Lock` | Toggle **hotlock** — freezes all trading keys so stray keypresses can't place or cancel orders while you're browsing. The 💰 dashboard shows a small lock icon while hotlock is active. |

### 🖱 Navigation & UI
| Action | Shortcut | Note |
| :--- | :--- | :--- |
| **Sort Column** | `Click Header` | Click again to reverse direction |
| **Quick Order Bar**| `Left Ctrl` / `Right Ctrl` | Toggles the order entry bar in the Market window (Left = BUY, Right = SELL) |
| **Search Symbol** | *(Market window)* | Opens a popup list to jump to another symbol |

### 🏦 Market
| Key | Action |
| :--- | :--- |
| `Esc` | Cancel **all** open orders for the current symbol |
| `Tab` | Jump from the list into the order-bar price field |
| `Left Ctrl` / `Right Ctrl` | Toggle the BUY / SELL quick-order bar (pre-filled with best bid/ask) |
| `Up / Down` | Change the order-bar price |
| `Shift` + `Up / Down` | Step price by **1.0** instead of 0.01 |

### 📝 Orders
| Key | Action |
| :--- | :--- |
| `Click` / Select an order | Opens the inline edit panel at the bottom (shown automatically for editable orders) |
| `Tab` | Cycle between the Price and Quantity fields |
| `Enter` | Confirm and submit the modification |
| `Esc` | Cancel the selected order (or discard the edit panel) |
| `Up / Down` | Increment / decrement the focused value |
| `Ctrl` + `Up / Down` | Move the selected order up or down in the list |
| `Shift` + `Up / Down` | Step the price by 1.0 (Quantity steps by 1) |

> **Note:** The Orders list has no delete key. Cancelling an order is done with `Esc`, not `Delete`.

### 💎 Diamonds (right-click menu)
Right-click a position to open a context menu:
| Action |
| :--- |
| **Quick BUY 1** — placeholder BUY order for 1 share at the last price |
| **Quick SELL 1** — placeholder SELL order for 1 share at 2× the last price |
| **Move to Growth / High-Yield Dividends / Quarantine** — reassign the group |
| **Set Color** — pick one of six colors (Red, Green, Blue, Purple, Gold, Brown) or None |

---

## ⚙️ Technical Details

### Registry Storage
Preferences are persisted under: `HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor`

### Build Instructions (from a Linux host to a Windows target)
Cross-compile using `make`:
```sh
# Prerequisites: x86_64-w64-mingw32-g++ and make
$ make
# Output: bin/Trading-Floor.exe
```

> **Platform Compatibility:** This application currently only runs on Windows 7 or higher. Linux and macOS support are not yet available.

---

## 🛠 Troubleshooting
*   **No Connection?** Check if TWS/Gateway is running and "Allow connections from localhost" is checked.
*   **Wrong Path?** If the app can't find the gateway, it will prompt you to select the `.exe` path.
*   **Reset App?** Delete the registry root: `Computer\HKEY_CURRENT_USER\Software\ibkr-gateway-trading-floor`.

## ⚖️ License & Legal
- Provided as-is for educational and personal use.
- Interactive Brokers® and IB Gateway® are trademarks of Interactive Brokers.
- Use at your own risk; verify behavior before trading live.

