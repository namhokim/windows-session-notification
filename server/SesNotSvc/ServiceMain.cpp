#include <Windows.h>
#include <tchar.h>
#include "BroadcastTcpServer.h"
#include "MessageCreator.h"

#define DEFAULT_LISTEN_PORT			13389
#define	DEFAULT_INTERESTING_PORT	3389

unsigned short ListenPort = DEFAULT_LISTEN_PORT;
unsigned short InterestingPort = DEFAULT_INTERESTING_PORT;

#ifdef _DEBUG
#define LOGMESSAGE( str ) OutputDebugString( str );
#else
#define LOGMESSAGE( str )
#endif

typedef struct _ServerAndMessage{
	BroadcastTcpServer* server;
	MessageCreator* message;
} ServerAndMessage, *PServerAndMessage;

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceCtrlHandlerEx(DWORD, DWORD, LPVOID, LPVOID);

#define SERVICE_NAME  _T("SCNS: Session Changing Notification Service")

int _tmain(int argc, TCHAR *argv[])
{
	if (argc == 3) {
		ListenPort = _tstoi(argv[1]);
		InterestingPort = _tstoi(argv[2]);
	}

	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{ SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
	{
		LOGMESSAGE(_T("SCNS: Main: StartServiceCtrlDispatcher returned error"));
		return GetLastError();
	}

	LOGMESSAGE(_T("SCNS: Main: Exit"));
	return 0;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
	DWORD Status = E_FAIL;

	LOGMESSAGE(_T("SCNS: ServiceMain: Entry"));

	BroadcastTcpServer server;
	MessageCreator mc(InterestingPort);
	ServerAndMessage sam;
	sam.message = &mc;
	sam.server = &server;
	server.listen(ListenPort);

	//g_StatusHandle = RegisterServiceCtrlHandler (SERVICE_NAME, ServiceCtrlHandler);
	g_StatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, ServiceCtrlHandlerEx, &sam);

	if (g_StatusHandle == NULL)
	{
		LOGMESSAGE(_T("SCNS: ServiceMain: RegisterServiceCtrlHandler returned error"));
		goto EXIT;
	}

	// Tell the service controller we are starting
	ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		LOGMESSAGE(_T("SCNS: ServiceMain: SetServiceStatus returned error"));
	}

	/*
	* Perform tasks neccesary to start the service here
	*/
	LOGMESSAGE(_T("SCNS: ServiceMain: Performing Service Start Operations"));

	// Create stop event to wait on later.
	g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (g_ServiceStopEvent == NULL)
	{
		LOGMESSAGE(_T("SCNS: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error"));

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		g_ServiceStatus.dwWin32ExitCode = GetLastError();
		g_ServiceStatus.dwCheckPoint = 1;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			LOGMESSAGE(_T("SCNS: ServiceMain: SetServiceStatus returned error"));
		}
		goto EXIT;
	}

	// Tell the service controller we are started
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
	g_ServiceStatus.dwCheckPoint = 0;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		LOGMESSAGE(_T("SCNS: ServiceMain: SetServiceStatus returned error"));
	}

	WaitForSingleObject(g_ServiceStopEvent, INFINITE);
	CloseHandle(g_ServiceStopEvent);

	g_ServiceStatus.dwControlsAccepted = 0;
	g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
	g_ServiceStatus.dwWin32ExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 3;

	if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
	{
		LOGMESSAGE(_T("SCNS: ServiceMain: SetServiceStatus returned error"));
	}

EXIT:
	LOGMESSAGE(_T("SCNS: ServiceMain: Exit"));

	return;
}

void NoticeSessionChanging(DWORD code, LPVOID lpContext) {
	ServerAndMessage* sam = (ServerAndMessage*)lpContext;
	if (sam->server->hasSubscriber()) {
		sam->server->broadcast(sam->message->makeMessage(code).c_str());
	}
}

VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{
	LOGMESSAGE(_T("SCNS: ServiceCtrlHandler: Entry"));

	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:

		LOGMESSAGE(_T("SCNS: ServiceCtrlHandler: SERVICE_CONTROL_STOP Request"));

		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		* Perform tasks neccesary to stop the service here
		*/

		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			LOGMESSAGE(_T("SCNS: ServiceCtrlHandler: SetServiceStatus returned error"));
		}

		// This will signal the worker thread to start shutting down
		SetEvent(g_ServiceStopEvent);

		break;

	default:
		break;
	}

	LOGMESSAGE(_T("SCNS: ServiceCtrlHandler: Exit"));
}

DWORD WINAPI ServiceCtrlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {

	if (dwControl == SERVICE_CONTROL_SESSIONCHANGE) {
		NoticeSessionChanging(dwEventType, lpContext);
	}
	else {
		ServiceCtrlHandler(dwControl);
	}

	return NO_ERROR;
}
