#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

#include "EWrapper.h"
#include "EClientSocket.h"
#include "EReaderOSSignal.h"
#include "EReader.h"

#define WM_API_UPDATE (WM_USER + 2)
#define WM_SYMBOL_RESULTS (WM_USER + 3)
#define TIMER_WATCHDOG 1

#define ID_M_SYMBOLS    1001
#define ID_M_CONNECT    3001
#define ID_M_DISCONNECT 3002
#define ID_M_EXIT       3003

bool shouldBeConnected = true;

class TradingAPI : public EWrapper {
private:
    HWND hTargetWnd = nullptr;
    
    EClientSocket* client;
    EReaderOSSignal signal;
    std::unique_ptr<EReader> reader;
    std::thread apiThread;
    std::atomic<bool> isThreadRunning;

    std::mutex dataMutex;

    std::string accountId;
    
    HWND hSymbolSearchWnd = nullptr; // separate target for symbol results
    std::mutex symbolMutex;
    std::vector<std::string> symbolResults;

    void processMessages() {
        while (isThreadRunning && client->isConnected()) {
            signal.waitForSignal();
            reader->processMsgs();
        }
    }

public:
    TradingAPI() : isThreadRunning(false), accountId("") {
        client = new EClientSocket(this, &signal);
    }

    ~TradingAPI() {
        disconnect();
        delete client;
    }
    
    void setWindowHandle(HWND hWnd) { hTargetWnd = hWnd; }

    bool connect() {
        if (client->eConnect("127.0.0.1", 4001, 0)) {
            reader = std::make_unique<EReader>(client, &signal);
            reader->start();
            isThreadRunning = true;
            apiThread = std::thread(&TradingAPI::processMessages, this);
            return true;
        }
        return false;
    }

    void disconnect() {
        isThreadRunning = false;
        if (client->isConnected())
            client->eDisconnect();
        if (apiThread.joinable()) {
            signal.issueSignal();
            apiThread.join();
        }
        reader.reset();
    }

    bool isConnected() const { return client && client->isConnected(); }

    // ── Callbacks you actually care about ────────────────────────────────────
    void connectAck() override {
        // Safe empty implementation override, but triggers a UI refresh notice
        if (hTargetWnd) {
            PostMessage(hTargetWnd, WM_API_UPDATE, 0, 0);
        }
    }

    void connectionClosed() override {
        if (hTargetWnd) {
            PostMessage(hTargetWnd, WM_API_UPDATE, 0, 0);
        }
    }

    void error(int id, time_t errorTime, int errorCode,
               const std::string& errorString,
               const std::string& advancedOrderRejectJson) override {
        if (errorCode != 2104 && errorCode != 2106 && errorCode != 2158) {
            std::cerr << "API Error [" << errorCode << "]: " << errorString;
            if (!advancedOrderRejectJson.empty())
                std::cerr << " | " << advancedOrderRejectJson;
            std::cerr << std::endl;
        }
    }

    std::string getAccountNumber() {
        std::lock_guard<std::mutex> lock(dataMutex);
        return accountId;
    }

    void managedAccounts(const std::string& accountsList) override {
        std::lock_guard<std::mutex> lock(dataMutex);
        accountId = accountsList;

        if (hTargetWnd) {
            PostMessage(hTargetWnd, WM_API_UPDATE, 0, 0);
        }
    }


    void setSymbolSearchWindow(HWND hWnd) { hSymbolSearchWnd = hWnd; }

    void searchSymbols(const std::string& pattern) {
        if (!client->isConnected()) return;
        client->reqMatchingSymbols(9001, pattern);
    }

    std::vector<std::string> getSymbolResults() {
        std::lock_guard<std::mutex> lock(symbolMutex);
        return symbolResults;
    }
    
    void symbolSamples(int reqId, const std::vector<ContractDescription>& contractDescriptions) override {
        std::lock_guard<std::mutex> lock(symbolMutex);
        symbolResults.clear();
        for (const auto& cd : contractDescriptions) {
            // Only stocks and ETFs
            if (cd.contract.secType == "STK" || cd.contract.secType == "ETF") {
                symbolResults.push_back(cd.contract.symbol);
            }
        }
        if (hSymbolSearchWnd)
            PostMessage(hSymbolSearchWnd, WM_SYMBOL_RESULTS, 0, 0);
    }

    #define managedAccounts managedAccounts_ignored
    #define error error_ignored
    #define symbolSamples symbolSamples_ignored
    #define connectAck connectAck_ignored
    #define connectionClosed connectionClosed_ignored

    #define EWRAPPER_VIRTUAL_IMPL {}
    #include "EWrapper_prototypes.h"
    #undef EWRAPPER_VIRTUAL_IMPL

    #undef managedAccounts_ignored
    #undef error_ignored
    #undef symbolSamples_ignored
    #undef connectAck_ignored
    #undef connectionClosed_ignored
} api;