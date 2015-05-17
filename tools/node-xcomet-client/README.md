# xcomet-client

## Usage

```bash
$ npm install --save xcomet-client
```

```javascript
var xcomet_client = require('xcomet-client');

var node1 = new xcomet_client.NodeInfo('192.168.2.3', '127.0.0.1', 9000, 9001);
var node2 = new xcomet_client.NodeInfo('192.168.2.3', '127.0.0.1', 9002, 9003);
var node3 = new xcomet_client.NodeInfo('192.168.2.3', '127.0.0.1', 9004, 9005);

var client = new xcomet_client.XCometClient([node1, node2, node3]);

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
```

## Parameters

### NodeInfo

1. public host name, ** required **
2. local host name, ** required **
3. client port, ** required **
4. admin port, ** require **
5. if the node in local network, ** optional **, defalut is true

### XCometClient

1. Array of NodeInfo, ** required **
2. sharding option, ** optional **, see ```sharding``` module for detail

### XCometClient.shard

1. user_id, ** required **

return: NodeInfo instance

### XCometClient.pub

1. option, ** required **, {to: '', from: '', msg: '', ttl: ''}
   `ttl` field is optional
2. callback, ** required **
