var JStream = require('jstream');
var net = require('net');

var hawaii = '10.255.1.146';
var hawaiiOne = '10.255.1.183';
var serverName = 'hawaii';

var request = require('request');
var client = new net.Socket();
client.connect(14172, hawaii, function() {
	console.log('Connected');

	client.pipe(new JStream()).on('data', function(obj) {
    console.log(obj);
  });
});

client.on('close', function() {
	console.log('Connection closed');
});
