var net = require('net');
var JStream = require('jstream');


var SnsClient = function(ipAddr, name, messenger, ipNameMap) {
	this.ip = ipAddr;
	this.name = name;
	this.client = new net.Socket();
	this.messenger = messenger;
	this.ipNameMap = ipNameMap;
	this.connectStack = [];
	this.handler = function handler(obj) {
		console.log(obj);

		if (obj.type === 'WTS_SESSION_UNLOCK') {
			var addr = (typeof this.ipNameMap[obj.addrs[0]] === 'undefined') ?  obj.addrs[0] : this.ipNameMap[obj.addrs[0]];
			this.connectStack.push(addr);
			var msg = 'Using ' + this.name + ' by ' + addr;
			this.messenger.sendMessage(msg);
		} else if(obj.type === 'WTS_CONSOLE_DISCONNECT' && obj.addrs.length === 0) {
			var addr = this.connectStack.pop();
			if (addr === 'undefined') {
				addr = 'someone';
			}
			var msg = 'Release ' + this.name + ' by ' + addr;
			this.messenger.sendMessage(msg);
		}
	};
};

SnsClient.prototype.connect = function(serverPort) {
	var name = this.name;
	var ip = this.ip;
	var client = this.client;
	var me = this;

	console.log("try connect to " + name);

	client.connect(serverPort, ip, function() {
		console.log(name + ' Connected');

		client.pipe(new JStream()).on('data', function(obj) {
	    	me.handler(obj);
		});
	});

	client.on('close', function() {
		console.log('Connection closed');
	});
};

module.exports = SnsClient;