#include <stdio.h>
#include <iostream>
#include <algorithm>	// for copy
#include <iterator>		// for ostream_iterator
#include "NetStat.h"

int main()
{
	using namespace std;

	const unsigned int remoteDesktopPort = 3389;

	vector<string> remoteAddrs;

	/*set<TCP_STATE> excepts;
	excepts.insert(TCP_STATE_LISTEN);
	netstatOnlyTcp(3389, excepts, remoteAddrs);*/

	printf("error code: %d\n", netstatOnlyTcp(3389, remoteAddrs));
	printf("count: %d\n", remoteAddrs.size());
	copy(remoteAddrs.begin(), remoteAddrs.end(), ostream_iterator<string>(cout, " "));

	return 0;
}
