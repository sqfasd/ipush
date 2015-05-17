var XCometClient = require('./index').XCometClient;
var NodeInfo = require('./index').NodeInfo;

var node1 = new NodeInfo('192.168.2.3', '127.0.0.1', 9000, 9001);
var node2 = new NodeInfo('192.168.2.3', '127.0.0.1', 9002, 9003);
var node3 = new NodeInfo('192.168.2.3', '127.0.0.1', 9004, 9005);

var client = new XCometClient([node1, node2, node3]);

var toUser = process.argv[2];
var fromUser = process.argv[3];
var message = process.argv[4];

// get user shard info
var node_info = client.shard(toUser);
console.log('shard for the user is: ' + require('util').inspect(node_info));

// pub to user
client.pub({to: toUser, from: fromUser, msg: message}, function(err, resp) {
  if (err) {
    console.log('pub err: ' + err);
  } else {
    console.log('pub result: ' + resp);
  }
});

// pub a message with ttl
client.pub({to: toUser, from: fromUser, msg: message, ttl: 10}, function(err, resp) {
  if (err) {
    console.log('pub err: ' + err);
  } else {
    console.log('pub result: ' + resp);
  }
});
