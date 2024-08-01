// service.cpp

#include <windows.h>
#include <tchar.h>  // _T 매크로 사용을 위해 추가
#include <iostream>

SERVICE_STATUS g_ServiceStatus = {};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  _T("SoundRiver")

int _tmain(int argc, TCHAR* argv[])
{
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        { (LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        return GetLastError();
    }

    return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD Status = E_FAIL;

    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);

    if (g_StatusHandle == NULL)
    {
        return;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    /*
     * Perform tasks necessary to start the service here
     */
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
        }
        return;
    }

    // We report the running status to SCM.
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    // Start a thread that will perform the main task of the service
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    // Wait until our service is stopped
    WaitForSingleObject(hThread, INFINITE);

    /*
     * Perform any cleanup tasks
     */
    CloseHandle(g_ServiceStopEvent);

    // Tell the service controller we are stopped
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    return;
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        /*
         * Perform tasks necessary to stop the service here
         */

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        break;

    default:
        break;
    }
}

DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    //  Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        // Perform main service function here
        // Just wait and continue
        Sleep(3000);
    }

    return ERROR_SUCCESS;
}
