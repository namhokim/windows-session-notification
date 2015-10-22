#include "NetStat.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#define GET_REQUIRED_SIZE_ERROR		0L
#define	NO_ENTRY					0
#define	ADDRESS_SIZE				128

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// function declares
DWORD getRequiredSize();
DWORD getTcpTable(__in const DWORD size, __in const unsigned short port, __in const std::set<TCP_STATE>& except_states, __out std::vector<std::string>& remoteAddress);
std::string convertAddress(DWORD address);
void setToVectorAndReturnSize(__in std::set<std::string>& original, __out std::vector<std::string>& destination);

unsigned long netstatOnlyTcp(__in const unsigned short port, __out std::vector<std::string>& remoteAddress) {
	std::set<TCP_STATE> empty_except_states;
	return netstatOnlyTcp(port, empty_except_states, remoteAddress);
}

unsigned long netstatOnlyTcp(__in const unsigned short port, __in const std::set<TCP_STATE>& except_states, __out std::vector<std::string>& remoteAddress) {
	int count = 3;
	DWORD dwRetVal = NO_ERROR;
	do {
		DWORD size = getRequiredSize();
		if (size == GET_REQUIRED_SIZE_ERROR) {
			return NO_ENTRY;
		}

		dwRetVal = getTcpTable(size, port, except_states, remoteAddress);

		if (ERROR_INSUFFICIENT_BUFFER != dwRetVal) {
			return dwRetVal;
		}

		--count;
	} while (count > 0);

	return dwRetVal;
}

DWORD getRequiredSize() {
	MIB_TCPTABLE dummyTcpTable;
	DWORD size = 0;

	if (GetTcpTable(&dummyTcpTable, &size, TRUE) == ERROR_INSUFFICIENT_BUFFER) {
		return size;
	}
	return GET_REQUIRED_SIZE_ERROR;
}

inline bool noExistOn(__in const std::set<TCP_STATE>& except_states, __in const DWORD state) {
	return except_states.find((TCP_STATE)state) == except_states.end();
}

DWORD getTcpTable(__in const DWORD size, __in const unsigned short port, __in const std::set<TCP_STATE>& except_states, __out std::vector<std::string>& remoteAddress) {
	using namespace std;

	vector<BYTE> buffer(size);
	PMIB_TCPTABLE pTcpTable = (PMIB_TCPTABLE)&buffer[0];
	DWORD dwSize = size;
	DWORD dwRetVal = 0;

	if ((dwRetVal = GetTcpTable(pTcpTable, &dwSize, TRUE)) == NO_ERROR) {
		set<string> addresses;
		bool needAllState = except_states.empty();
		for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
			MIB_TCPROW& entry = pTcpTable->table[i];
			u_short localPort = ntohs((u_short)entry.dwLocalPort);
			if (localPort == port && (needAllState || noExistOn(except_states, entry.dwState))) {
				addresses.insert(convertAddress(entry.dwRemoteAddr));
			}
		}
		setToVectorAndReturnSize(addresses, remoteAddress);
	}

	return dwRetVal;
}

std::string convertAddress(DWORD address) {
	struct in_addr IpAddr;
	char remoteAddr[ADDRESS_SIZE];
	IpAddr.S_un.S_addr = (u_long)address;
	inet_ntop(AF_INET, &IpAddr, remoteAddr, ADDRESS_SIZE);
	return remoteAddr;
}

void setToVectorAndReturnSize(__in std::set<std::string>& original, __out std::vector<std::string>& destination) {
	destination.clear();
	destination.reserve(original.size());
	destination.assign(original.begin(), original.end());
}

