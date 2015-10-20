#include "BroadcastTcpServer.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#pragma comment(lib, "ws2_32.lib")

std::string getLastWsaError(const char *msg);

class TcpServerSocket {
public:
	TcpServerSocket();
	~TcpServerSocket();
	bool bind(unsigned short port);
	bool listen(int backlog);
	SOCKET accept(struct sockaddr* addr, int* addrlen);
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

	static DWORD WINAPI StaticThreadStart(void* Param)
	{
		TcpClientSubscriber* self = (TcpClientSubscriber*)Param;
		while (true) {
			SOCKET client_sock;
			SOCKADDR_IN clientaddr;
			int addrlen = sizeof(clientaddr);
			client_sock = self->tss->accept((SOCKADDR *)&clientaddr, &addrlen);
			if (client_sock != INVALID_SOCKET) {
				self->addSubscriber(client_sock);
			}
		}
	}

private:
	CRITICAL_SECTION criticalSection;
	TcpServerSocket * tss;
	std::vector<SOCKET> subscribers;
	HANDLE hThread;
	bool continueAccept;

	void disconnectSubscribers();
};


BroadcastTcpServer::BroadcastTcpServer() : tss(0), subscriber(0)
{
	// 윈속 초기화
	WSADATA wsa;
	this->winsockInitialized = (WSAStartup(MAKEWORD(2, 2), &wsa) == 0);

	this->tss = new TcpServerSocket();
	this->subscriber = new TcpClientSubscriber();
}


BroadcastTcpServer::~BroadcastTcpServer()
{
	close();
	if (this->winsockInitialized) {
		// 윈속 종료
		WSACleanup();

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
		this->subscriber = 0;
	}

	if (this->tss != 0) {
		delete this->tss;
		this->tss = 0;
	}
}



// TcpServerSocket implementation
TcpServerSocket::TcpServerSocket() : serverSocket(INVALID_SOCKET) {
	this->serverSocket = ::socket(AF_INET, SOCK_STREAM, 0);
}

TcpServerSocket::~TcpServerSocket() {
	if (this->serverSocket != INVALID_SOCKET) {
		::closesocket(this->serverSocket);
	}
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

void TcpServerSocket::recordMessage(const char *headerMessage)
{
	this->message.assign(getLastWsaError(headerMessage));
}


// 소켓 함수 오류 출력 후 종료
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


TcpClientSubscriber::TcpClientSubscriber()
{
	::InitializeCriticalSectionAndSpinCount(&criticalSection, 0x00000400);
}

TcpClientSubscriber::~TcpClientSubscriber()
{
	this->continueAccept = false;
	disconnectSubscribers();
	::DeleteCriticalSection(&criticalSection);
}

int TcpClientSubscriber::broadcast(const char * message)
{
	using namespace std;

	EnterCriticalSection(&criticalSection);
	vector<SOCKET> notifications(this->subscribers);
	this->subscribers.clear();
	LeaveCriticalSection(&criticalSection);

	vector<SOCKET> nextSubscribers(notifications.size());
	for (vector<SOCKET>::size_type pos = 0; pos < notifications.size(); pos++) {
		SOCKET s = notifications.at(pos);

		int retval = ::send(s, message, strlen(message), 0);
		if (retval == SOCKET_ERROR) {
			::closesocket(s);
			break;
		}
		// TODO - check send bytes equals to length of message
		nextSubscribers.push_back(s);
	}

	EnterCriticalSection(&criticalSection);
	this->subscribers.insert(this->subscribers.end(), nextSubscribers.begin(), nextSubscribers.end());
	LeaveCriticalSection(&criticalSection);

	return nextSubscribers.size();
}

void TcpClientSubscriber::addSubscriber(SOCKET socket)
{
	EnterCriticalSection(&criticalSection);
	this->subscribers.push_back(socket);
	LeaveCriticalSection(&criticalSection);
}

void TcpClientSubscriber::acceptAsync(TcpServerSocket * tss)
{
	this->tss = tss;
	this->continueAccept = true;

	DWORD ThreadID;
	this->hThread = CreateThread(NULL, 0, StaticThreadStart, (LPVOID)this, 0, &ThreadID);
}

void TcpClientSubscriber::disconnectSubscribers()
{
	using namespace std;

	EnterCriticalSection(&criticalSection);
	vector<SOCKET> notifications(this->subscribers);
	this->subscribers.clear();
	LeaveCriticalSection(&criticalSection);

	for (vector<SOCKET>::size_type pos = 0; pos < notifications.size(); pos++) {
		SOCKET s = notifications.at(pos);
		::closesocket(s);
	}
}
