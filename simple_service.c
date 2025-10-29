#include <windows.h>
#include <stdio.h>

SERVICE_STATUS_HANDLE g_StatusHandle;
HANDLE g_StopEvent;

VOID WINAPI ServiceMain(DWORD argc, LPSTR *argv);
VOID WINAPI Handler(DWORD control);

int main()
{
    SERVICE_TABLE_ENTRY table[] = {
        { "SimpleService", ServiceMain },
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