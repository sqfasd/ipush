var url = require('url');
var querystring = require('querystring');
var request = require('request');
var Sharding = require('sharding').Sharding;

module.exports = {
  NodeInfo: NodeInfo,
  XCometAdmin: XCometAdmin,
}

function NodeInfo(public_host,
                  local_host,
                  client_port,
                  admin_port,
                  not_local) {
  this._public_host = public_host;
  this._local_host = local_host;
  this._client_port = client_port;
  this._admin_port = admin_port;
  this._not_local = not_local || false;
}

NodeInfo.prototype.getPublicHost = function() {
  return this._public_host;
}

NodeInfo.prototype.getLocalHost = function() {
  return this._local_host;
}

NodeInfo.prototype.getClientPort = function() {
  return this._client_port;
}

NodeInfo.prototype.getAdminPort = function() {
  return this._admin_port;
}

NodeInfo.prototype.notLocal = function() {
  return this._not_local;
}

function XCometAdmin(nodes, option) {
  this._sharding = new Sharding(nodes, option);

  // nodes has been Array and legnth greater than 0
  for (var i in nodes) {
    if (!(nodes[i] instanceof NodeInfo)) {
      throw new Error('all nodes must be instance of NodeInfo');
    }
  }
}

XCometAdmin.prototype.shard = function(user) {
  return this._sharding.shard(user);
}

XCometAdmin.prototype.pub = function(option, callback) {
  if (!(option && option.to && option.from && option.msg)) {
    throw new Error('invalid parameters');
  }
  var to = option.to;
  var from = option.from;
  var msg = option.msg;
  if (typeof msg === 'object') {
    if (msg instanceof String) {
      msg = msg.toString();
    } else {
      msg = JSON.stringify(msg);
    }
  }
  if (typeof msg !== 'string') {
    throw new Error("parameter 'msg' must be string type");
  }
  var node_info = this.shard(to);
  var url_obj = {};
  url_obj.protocol = 'http';
  if (node_info.notLocal()) {
    url_obj.hostname = node_info.getPublicHost();
  } else {
    url_obj.hostname = node_info.getLocalHost();
  }
  url_obj.port = node_info.getAdminPort();
  url_obj.pathname = '/pub';
  var query = {to: to, from: from};
  if (option.ttl) {
    query.ttl = option.ttl;
  }
  url_obj.search = querystring.stringify(query);

  var url_str = url.format(url_obj);
  //console.log('url = ' + url_str);
  request.post(url_str, {form: msg}, function(err, resp, body) {
    if (err) {
      //console.log('err = ' + err);
      callback(new Error('pub request failed: ' + err));
    } else if (resp.statusCode !== 200) {
      //console.log('statusCode = ' + resp.sttusCode);
      //console.log('body = ' + body);
      callback(body);
    } else {
      //console.log('body = ' + body);
      callback(null, body);
    }
  });
}
