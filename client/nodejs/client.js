var snsClient = require('./SnsClient');
var slackBot = require('./slackMessage');
var config = require('./config');	// using config.json


var slack = new slackBot({
	team: config.slack.team,
	token: config.slack.token,
	channel: config.slack.channel
});

const serverPort  = 14172;

for (ip in config.notiServers) {
	var client = new snsClient(ip, config.notiServers[ip], slack, config.ipNameMap);
	client.connect(serverPort);
}
