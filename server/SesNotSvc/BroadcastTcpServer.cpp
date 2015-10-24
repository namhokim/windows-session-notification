#include "BroadcastTcpServer.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <list>
#include <process.h>	// _beginthreadex
#pragma comment(lib, "ws2_32.lib")

const static int DeletedObject = 0;

std::string getLastWsaError(const char *msg);

class TcpServerSocket {
public:
	TcpServerSocket();
	~TcpServerSocket();
	bool bind(unsigned short port);
	bool listen(int backlog);
	SOCKET accept(struct sockaddr* addr, int* addrlen);
	void close();
private:
	SOCKET serverSocket;
	std::string message;
	void recordMessage(const char*);
};

class TcpClientSubscriber {
public:
	TcpClientSubscriber();
	~TcpClientSubscriber();
	int broadcast(const char* message);
	void addSubscriber(SOCKET socket);
	void acceptAsync(TcpServerSocket* tss);
	void stopAcceptAsync() {
		this->continueAccept = false;
	}
	bool hasSubscriber();

	static unsigned __stdcall asyncAcceptThreadFunction(void* Param)
	{
		TcpClientSubscriber* self = (TcpClientSubscriber*)Param;
		::ResetEvent(self->threadStopEvent);	// nonsignaled
		while (self->continueAccept) {
			SOCKADDR_IN clientaddr;
			int addrlen = sizeof(clientaddr);
			if (self->tss == DeletedObject) {
				return 0;
			}
			SOCKET client_sock = self->tss->accept((SOCKADDR *)&clientaddr, &addrlen);
			if (client_sock != INVALID_SOCKET) {
				if (self->continueAccept) {
					self->addSubscriber(client_sock);
				}
			}
		}
		::SetEvent(self->threadStopEvent);	// signaled
		return 0;
	}

private:
	CRITICAL_SECTION criticalSection;
	TcpServerSocket * tss;
	std::list<SOCKET> subscribers;
	volatile bool continueAccept;	// use volatile for multi-thread check
	HANDLE threadStopEvent;

	void disconnectSubscribers();
	void stopServer();
};


BroadcastTcpServer::BroadcastTcpServer() : tss(DeletedObject), subscriber(DeletedObject)
{
	// 윈속 초기화
	WSADATA wsa;
	this->winsockInitialized = (::WSAStartup(MAKEWORD(2, 2), &wsa) == 0);

	this->tss = new TcpServerSocket();
	this->subscriber = new TcpClientSubscriber();
}


BroadcastTcpServer::~BroadcastTcpServer()
{
	close();
	if (this->winsockInitialized) {
		// 윈속 종료
		::WSACleanup();

		this->winsockInitialized = false;
	}
}

void BroadcastTcpServer::listen(unsigned short port)
{
	// bind
	if (this->tss->bind(port)) {
		// listen
		if (this->tss->listen(SOMAXCONN)) {
			this->subscriber->acceptAsync(this->tss);
		}
	}
}

int BroadcastTcpServer::broadcast(const char * message)
{
	return subscriber->broadcast(message);
}

void BroadcastTcpServer::close()
{
	if (this->subscriber != 0) {
		delete this->subscriber;
		this->subscriber = DeletedObject;
	}

	if (this->tss != 0) {
		delete this->tss;
		this->tss = DeletedObject;
	}
}

bool BroadcastTcpServer::hasSubscriber()
{
	return subscriber->hasSubscriber();
}



// TcpServerSocket implementation
TcpServerSocket::TcpServerSocket() : serverSocket(INVALID_SOCKET) {
	this->serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);
}

TcpServerSocket::~TcpServerSocket() {
	close();
}

bool TcpServerSocket::bind(unsigned short port)
{
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int retval = ::bind(serverSocket, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		this->recordMessage("bind()");
		return false;
	}
	return true;
}

bool TcpServerSocket::listen(int backlog)
{
	int retval = ::listen(serverSocket, backlog);
	if (retval == SOCKET_ERROR) {
		this->recordMessage("listen()");
		return false;
	}
	return true;
}

SOCKET TcpServerSocket::accept(sockaddr * addr, int * addrlen)
{
	return ::accept(this->serverSocket, addr, addrlen);
}

void TcpServerSocket::close()
{
	if (this->serverSocket != INVALID_SOCKET) {
		::closesocket(this->serverSocket);
		this->serverSocket = INVALID_SOCKET;
	}
}

void TcpServerSocket::recordMessage(const char *headerMessage)
{
	this->message.assign(getLastWsaError(headerMessage));
}


// 소켓 함수 에러메세지 획득
std::string getLastWsaError(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf, 0, NULL);
	std::string message((LPSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);

	return message;
}


TcpClientSubscriber::TcpClientSubscriber() : threadStopEvent(INVALID_HANDLE_VALUE), tss(DeletedObject)
{
	::InitializeCriticalSection(&criticalSection);
	threadStopEvent = ::CreateEvent(
		NULL,	// default security attributes
		TRUE,	// manual-reset event
		TRUE,	// initial state is signaled
		NULL);
}

TcpClientSubscriber::~TcpClientSubscriber()
{
	stopAcceptAsync();
	stopServer();
	disconnectSubscribers();
	::WaitForSingleObject(this->threadStopEvent, INFINITE);
	::CloseHandle(this->threadStopEvent);
	::DeleteCriticalSection(&criticalSection);
}

int TcpClientSubscriber::broadcast(const char * message)
{
	const int CannotSend = 0;
	using namespace std;

	int succeedCount = 0;
	::EnterCriticalSection(&criticalSection);
	list<SOCKET>::iterator pos = this->subscribers.begin();
	while (pos != this->subscribers.end()) {
		SOCKET s = *pos;
		int retval = ::send(s, message, (int)strlen(message), 0);
		if (retval == SOCKET_ERROR || retval == CannotSend) {
			::shutdown(s, SD_SEND);
			::closesocket(s);
			this->subscribers.erase(pos++);
		}
		else {
			succeedCount++;
			pos++;
		}
	}
	::LeaveCriticalSection(&criticalSection);

	return succeedCount;
}

void TcpClientSubscriber::addSubscriber(SOCKET socket)
{
	::EnterCriticalSection(&criticalSection);
	this->subscribers.push_back(socket);
	::LeaveCriticalSection(&criticalSection);
}

void TcpClientSubscriber::acceptAsync(TcpServerSocket * tss)
{
	const int BeginThreadExFailed = 0;

	this->tss = tss;
	this->continueAccept = true;

	unsigned int threadID;
	HANDLE hThread = (HANDLE)::_beginthreadex(NULL, 0, asyncAcceptThreadFunction, (LPVOID)this, 0, &threadID);
	if (hThread != BeginThreadExFailed) {
		::CloseHandle(hThread);
	}
}

bool TcpClientSubscriber::hasSubscriber()
{
	::EnterCriticalSection(&criticalSection);
	bool has = !(this->subscribers.empty());
	::LeaveCriticalSection(&criticalSection);
	return has;
}

void TcpClientSubscriber::disconnectSubscribers()
{
	using namespace std;

	::EnterCriticalSection(&criticalSection);
	list<SOCKET>::iterator pos = this->subscribers.begin();
	while (pos != this->subscribers.end()) {
		SOCKET s = *pos;
		::closesocket(s);
		this->subscribers.erase(pos++);
	}
	::LeaveCriticalSection(&criticalSection);
}

void TcpClientSubscriber::stopServer()
{
	if (tss != DeletedObject) {
		tss->close();
	}
}
