if (obj.type === 'WTS_SESSION_UNLOCK') {
	var ip = (obj.addrs.length > 0) ? obj.addrs[0] : "";
	console.log(serverName + ' was using by ' + ip);
} else if (obj.type === 'WTS_CONSOLE_DISCONNECT') {
	
}