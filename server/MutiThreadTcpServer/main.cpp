#include <Windows.h>
#include "BroadcastTcpServer.h"

int main(int argc, char* argv[])
{
	const int TenSeconds = 10000;
	const int ThirtySeconds = 30000;
	const int SixtySeconds = 60000;

	BroadcastTcpServer server;
	server.listen(9000);
	Sleep(TenSeconds);
	server.broadcast("Message");
	Sleep(ThirtySeconds);

	return 0;
}
