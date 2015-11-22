var net = require('net');
var JStream = require('jstream');

const MilliSeconds = 1;
const Seconds = 1000 * MilliSeconds;
const Minutes = 60 * Seconds;
const ThirtyMinutes = 30 * Minutes;
const FiveMinutes = 5 * Minutes;
var timeoutConfig = FiveMinutes;

function popFrom(map, defaultValue) {
	var poppedValue = map.pop();
	if (typeof map !== 'object' || typeof poppedValue === 'undefined') {
		return defaultValue;
	}
	return poppedValue;
}

function padStr(i) {
    return (i < 10) ? "0" + i : "" + i;
}

function getToday() {
	var date = new Date();
	return padStr(date.getFullYear()) + "-" +
                  padStr(1 + date.getMonth()) + "-" +
                  padStr(date.getDate()) + " " +
                  padStr(date.getHours()) + ":" +
                  padStr(date.getMinutes()) + ":" +
                  padStr(date.getSeconds());
}

var SnsClient = function(ipAddr, name, messenger, ipNameMap) {
	this.ip = ipAddr;
	this.name = name;
	this.messenger = messenger;
	this.ipNameMap = ipNameMap;
	this.connectStack = [];
	this.handler = function handler(obj) {
	    console.log(obj);

		if (obj.type === 'WTS_SESSION_UNLOCK') {
		    var addr = ipNameMap.get(obj.addrs[0]);
		    console.log(addr);
			this.connectStack.push(addr);
			var msg = 'Connect to ' + this.name + ' by ' + addr + ' , ' + getToday();
			this.messenger.sendMessage(msg);
		} else if(obj.type === 'WTS_CONSOLE_DISCONNECT' && obj.addrs.length === 0) {
			var addr = popFrom(this.connectStack, 'someone');
			var msg = 'Disconnect to ' + this.name + ' by ' + addr + ' , ' + getToday();
			this.messenger.sendMessage(msg);
		}
	};
};

SnsClient.prototype.connect = function(serverPort) {
	var name = this.name;
	var ip = this.ip;
	var client = new net.Socket({
		allowHalfOpen: true,
		readable: true,
		writable: false
	});
	var self = this;
	
	client.setTimeout(timeoutConfig);

	console.log("try connect to " + name);

	client.connect(serverPort, ip, function() {
		console.log(name + ' Connected');

		client.pipe(new JStream()).on('data', function(obj) {
	    	self.handler(obj);
		});
	});

	client.on('close', function() {
		console.log('Connection closed');
		self.connect(serverPort);
	});

	client.on('timeout', function() {
		console.log("Idle timeout");
		client.end();
		client.destroy();
	});

	client.on('error', function(obj) {
		console.log(obj);
	});
};

module.exports = SnsClient;
