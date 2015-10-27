var snsClient = require('./SnsClient');
var slackBot = require('./slackMessage');

// configurations
var notiServers = {
	'10.11.22.1': 'Server 1',
	'10.11.22.2': 'Server 2'
};

var slack = new slackBot({
	team: "hawaii",
	token: "f5rfwj5uorPWeJDnEkp6ApUJ",
	channel: "test"
});

var ipNameMap = {
	'192.168.1.2': 'Raphael(wi-fi)',
	'192.168.1.3': 'Gyeomgun(wi-fi)',
	'192.168.1.4': 'Raphael(wire)'
};

var serverPort  = 14172;


for (ip in notiServers) {
	var client = new snsClient(ip, notiServers[ip], slack, ipNameMap);
	client.connect(serverPort);
}
