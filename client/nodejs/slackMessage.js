var https = require('https');

function isNotValidateParam(configure) {
	return (
		typeof configure === 'undefined' ||
		typeof configure.team === 'undefined' ||
		typeof configure.token === 'undefined' ||
		typeof configure.channel === 'undefined');
}

function SlackMessage(configure) {
	this.notInitizalized = isNotValidateParam(configure);
	if (this.notInitizalized) {
		return 'example {team: "team", token: "token", channel: "channel"}';
	}

	this.options = {
	  hostname: configure.team + '.slack.com',
	  port: 443,
	  path: '/services/hooks/slackbot?token=' + configure.token + '&channel=%23' + configure.channel,
	  method: 'POST',
	  headers: {
	  	'Content-Type': 'plain/text; charset=utf-8'
	  }
	};
}

SlackMessage.prototype.sendMessage = function(msg) {
	this.options.headers['Content-Length'] = Buffer.byteLength(msg, 'utf8');

	var req = https.request(this.options);
	req.write(msg);
	req.end();

	req.on('error', function(e) {
	  console.error(e);
	});
};

module.exports = SlackMessage;
