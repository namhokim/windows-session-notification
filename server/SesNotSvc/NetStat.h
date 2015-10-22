#pragma once

#include <string>
#include <set>
#include <vector>

typedef enum {
	TCP_STATE_CLOSED = 1,
	TCP_STATE_LISTEN = 2,
	TCP_STATE_SYN_SENT = 3,
	TCP_STATE_SYN_RCVD = 4,
	TCP_STATE_ESTAB = 5,
	TCP_STATE_FIN_WAIT1 = 6,
	TCP_STATE_FIN_WAIT2 = 7,
	TCP_STATE_CLOSE_WAIT = 8,
	TCP_STATE_CLOSING = 9,
	TCP_STATE_LAST_ACK = 10,
	TCP_STATE_TIME_WAIT = 11,
	TCP_STATE_DELETE_TCB = 12,
} TCP_STATE;

unsigned long netstatOnlyTcp(__in const unsigned short port, __out std::vector<std::string>& remoteAddress);
unsigned long netstatOnlyTcp(__in const unsigned short port, __in const std::set<TCP_STATE>& except_states, __out std::vector<std::string>& remoteAddress);