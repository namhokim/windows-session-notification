#pragma once

// Pimpl (pointer to implementation) idiom.
class TcpServerSocket;
class TcpClientSubscriber;

class BroadcastTcpServer
{
public:
	BroadcastTcpServer();
	~BroadcastTcpServer();
	void listen(unsigned short port);
	int broadcast(const char* message);
	void close();
	bool hasSubscriber();
private:
	bool winsockInitialized;
	TcpServerSocket* tss;
	TcpClientSubscriber* subscriber;
};

