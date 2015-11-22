var snsClient = require('./SnsClient');
var slackBot = require('./slackMessage');
var fileConfig = require('./fileConfig');
var keyValueMap = require('./kvMap');

var ipNameMap = new keyValueMap();
var config = new fileConfig('config', function (newValue) {
    var newIpNameMap = newValue.ipNameMap;
    console.log("configure changed: " + newIpNameMap);
    ipNameMap.set(newIpNameMap);
});
ipNameMap.set(config.ipNameMap);

var slack = new slackBot({
	team: config.slack.team,
	token: config.slack.token,
	channel: config.slack.channel
});

const serverPort = 14172;

for (ip in config.notiServers) {
    var client = new snsClient(ip, config.notiServers[ip], slack, ipNameMap);
	client.connect(serverPort);
}
