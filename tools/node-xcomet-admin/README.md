# xcomet-admin

## Usage

```bash
$ npm install --save xcomet-admin
```

```javascript
var xcomet_admin = require('xcomet-admin');

var node1 = new xcomet_admin.NodeInfo('192.168.2.3', '127.0.0.1', 9000, 9001);
var node2 = new xcomet_admin.NodeInfo('192.168.2.3', '127.0.0.1', 9002, 9003);
var node3 = new xcomet_admin.NodeInfo('192.168.2.3', '127.0.0.1', 9004, 9005);

var admin = new xcomet_admin.XCometAdmin([node1, node2, node3]);

var toUser = process.argv[2];
var fromUser = process.argv[3];
var message = process.argv[4];

// get user shard info
var node_info = admin.shard(toUser);
console.log('shard for the user is: ' + require('util').inspect(node_info));

// pub to user
admin.pub({to: toUser, from: fromUser, msg: message}, function(err, resp) {
  if (err) {
    console.log('pub err: ' + err);
  } else {
    console.log('pub result: ' + resp);
  }
});

// pub a message with ttl
admin.pub({to: toUser, from: fromUser, msg: message, ttl: 10}, function(err, resp) {
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

### XCometAdmin

1. Array of NodeInfo, ** required **
2. sharding option, ** optional **, see ```sharding``` module for detail

### XCometAdmin.shard

1. user_id, ** required **

return: NodeInfo instance

### XCometAdmin.pub

1. option, ** required **, {to: '', from: '', msg: '', ttl: ''}
   `ttl` field is optional
2. callback, ** required **
