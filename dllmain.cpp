// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "CSocketProvider.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

static CSocketProvider SocketProvider;

int 
WSPAPI
WSPStartup(
    WORD                wVersion,
    LPWSPDATA           lpWSPData,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    WSPUPCALLTABLE      UpCallTable,
    LPWSPPROC_TABLE     lpProcTable
)
{
    if (0 != SocketProvider.Initialize(wVersion, lpWSPData, lpProtocolInfo, UpCallTable, lpProcTable))
        return WSAEPROVIDERFAILEDINIT;

    return 0;
}

void
WSPAPI
GetLspGuid(
    LPGUID lpGuid
)
{
    memcpy(lpGuid, &gProviderGuid, sizeof(GUID));
}