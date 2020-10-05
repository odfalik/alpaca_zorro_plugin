// AlpacaZorroPlugin.cpp : Defines the exported functions for the DLL application.
//
// Alpaca plugin for Zorro Automated Trading System
// Written by Kun Zhao
// 
// Alpaca - a modern platform for algorithmic trading. (https://alpaca.markets/)
//

#include "stdafx.h"

#include "AlpacaZorroPlugin.h"

// standard library
#include <string>
#include <sstream>
#include <vector>
#include <iomanip> // setprecision
#include <chrono>
#include <thread>
#include <algorithm> // transform

#include "alpaca/client.h"
#include "include/functions.h"

#define PLUGIN_VERSION	2

namespace alpaca
{
    std::unique_ptr<Client> client = nullptr;

    ////////////////////////////////////////////////////////////////
    DLLFUNC_C int BrokerOpen(char* Name, FARPROC fpError, FARPROC fpProgress)
    {
        strcpy_s(Name, 32, "Alpaca");
        (FARPROC&)BrokerError = fpError;
        (FARPROC&)BrokerProgress = fpProgress;
        return PLUGIN_VERSION;
    }

    DLLFUNC_C void BrokerHTTP(FARPROC fpSend, FARPROC fpStatus, FARPROC fpResult, FARPROC fpFree)
    {
        (FARPROC&)http_send = fpSend;
        (FARPROC&)http_status = fpStatus;
        (FARPROC&)http_result = fpResult;
        (FARPROC&)http_free = fpFree;
        return;
    }

    DLLFUNC_C int BrokerLogin(char* User, char* Pwd, char* Type, char* Account)
    {
        if (!User) // log out
        {
            return 0;
        }

        bool isPaperTrading = strcmp(Type, "Demo") == 0;
        client = std::make_unique<Client>(User, Pwd, isPaperTrading);

        //client->getClock();

        //attempt login
        auto response = client->getAccount();
        if (!response) {
            BrokerError("Login failed.");
            BrokerError(response.what().c_str());
            return 0;
        }

        auto& account = response.content().account_number;
        BrokerError(("Account " + account).c_str());
        sprintf_s(Account, 1024, account.c_str());
        return 1;
    }

    DATE convertTime(__time32_t t32)
    {
        return (DATE)t32 / (24. * 60. * 60.) + 25569.; // 25569. = DATE(1.1.1970 00:00)
    }

    __time32_t convertTime(DATE date)
    {
        return (__time32_t)((date - 25569.) * 24. * 60. * 60.);
    }

    DLLFUNC_C int BrokerTime(DATE* pTimeGMT)
    {
        int retry = 10;
        while (retry--) {
            auto response = client->getClock();
            if (!response) {
#ifdef _DEBUG
                BrokerError(response.what().c_str());
#endif
                Sleep(100);
                continue;
            }

            auto& clock = response.content();
            BrokerError(std::to_string(clock.timestamp).c_str());

            *pTimeGMT = convertTime(clock.timestamp);
            return clock.is_open ? 2 : 1;
        }
        return 0;
    }

    DLLFUNC_C int BrokerAsset(char* Asset, double* pPrice, double* pSpread, double* pVolume, double* pPip, double* pPipCost, double* pLotAmount, double* pMarginCost, double* pRollLong, double* pRollShort)
    {
        auto response = client->getAsset(Asset);
        if (!response) {
            return 0;
        }

        return 1;
    }

    DLLFUNC_C int BrokerHistory2(char* Asset, DATE tStart, DATE tEnd, int nTickMinutes, int nTicks, T6* ticks)
    {

        if (!client || !Asset || !ticks || !nTicks) return 0;

        auto response = client->getBars({ Asset }, "start", "", "", "", "1Min", nTicks);
        if (!response) {
            return 0;
        }

        uint32_t i = 0;
        for (auto& bar : response.content().bars[Asset]) {
            auto& tick = ticks[i++];
            tick.time = bar.time;
            tick.fOpen = bar.open_price;
            tick.fHigh = bar.high_price;
            tick.fLow = bar.low_price;
            tick.fClose = bar.close_price;
            tick.fVol = bar.volume;
        }
        return i;
    }

    DLLFUNC_C int BrokerAccount(char* Account, double* pdBalance, double* pdTradeVal, double* pdMarginVal)
    {
        auto response = client->getAccount();
        if (!response) {
            return 0;
        }

        if (strcmp(Account, response.content().account_number.c_str()) != 0) {
            assert(false);
            return 0;
        }

        *pdBalance = response.content().equity;
        return 1;
    }

    DLLFUNC_C int BrokerBuy2(char* Asset, int nAmount, double dStopDist, double dLimit, double* pPrice, int* pFill)
    {
        // TODO
        return 0;
    }

    DLLFUNC_C double BrokerCommand(int Command, DWORD dwParameter)
    {
        // TODO
        return .0;
    }
}
