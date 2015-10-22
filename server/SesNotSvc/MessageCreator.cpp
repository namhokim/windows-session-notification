#include "MessageCreator.h"
#include "NetStat.h"

/*
* codes passed in WPARAM for WM_WTSSESSION_CHANGE
*/

#define WTS_CONSOLE_CONNECT                0x1
#define WTS_CONSOLE_DISCONNECT             0x2
#define WTS_REMOTE_CONNECT                 0x3
#define WTS_REMOTE_DISCONNECT              0x4
#define WTS_SESSION_LOGON                  0x5
#define WTS_SESSION_LOGOFF                 0x6
#define WTS_SESSION_LOCK                   0x7
#define WTS_SESSION_UNLOCK                 0x8
#define WTS_SESSION_REMOTE_CONTROL         0x9
#define WTS_SESSION_CREATE                 0xa
#define WTS_SESSION_TERMINATE              0xb


MessageCreator::MessageCreator(unsigned short interestedPort) : port(interestedPort)
{
}


MessageCreator::~MessageCreator()
{
}

std::string MessageCreator::makeMessage(unsigned long code)
{
	using namespace std;

	vector<string> remoteAddrs;

	set<TCP_STATE> excepts;
	excepts.insert(TCP_STATE_LISTEN);
	netstatOnlyTcp(this->port, excepts, remoteAddrs);

	string message;
	message.reserve(1024);
	message.append("{\"type\":\"");
	message.append(codeToString(code));
	message.append("\", \"addrs\": [");
	message.append(addrsToString(remoteAddrs));
	message.append("]}");
	return message;
}

std::string MessageCreator::dwordToString(unsigned long dw)
{
	using namespace std;

	const int n = snprintf(NULL, 0, "%lu", dw);
	vector<char> buf(n + 1);
	if (n < 0) {
		return "";
	}
	int c = snprintf(&buf[0], n + 1, "%lu", dw);
	string str(&buf[0]);
	return str;
}

std::string MessageCreator::codeToString(unsigned long code)
{
	switch (code) {
	case WTS_CONSOLE_CONNECT:
		return "WTS_CONSOLE_CONNECT";
	case WTS_CONSOLE_DISCONNECT:
		return "WTS_CONSOLE_DISCONNECT";
	case WTS_REMOTE_CONNECT:
		return "WTS_REMOTE_CONNECT";
	case WTS_REMOTE_DISCONNECT:
		return "WTS_REMOTE_DISCONNECT";
	case WTS_SESSION_LOGON:
		return "WTS_SESSION_LOGON";
	case WTS_SESSION_LOGOFF:
		return "WTS_SESSION_LOGOFF";
	case WTS_SESSION_LOCK:
		return "WTS_SESSION_LOCK";
	case WTS_SESSION_UNLOCK:
		return "WTS_SESSION_UNLOCK";
	case WTS_SESSION_REMOTE_CONTROL:
		return "WTS_SESSION_REMOTE_CONTROL";
	default:
		return dwordToString(code);
	}
}

std::string MessageCreator::addrsToString(const std::vector<std::string>& addrs)
{
	if (addrs.empty()) {
		return "";
	}

	// xxx.xxx.xxx.xxx : xxx (3) * 4 + . (3)
	std::string buf;
	buf.reserve((3 * 4 + 3)*addrs.size());
	for (std::vector<std::string>::size_type pos = 0; pos < addrs.size(); pos++) {
		if (pos != 0) {
			buf.append(",");
		}
		buf.append("\"");
		buf.append(addrs.at(pos));
		buf.append("\"");
	}
	return buf;
}
