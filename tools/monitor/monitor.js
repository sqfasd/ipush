var util = require('util');
var path = require('path');
var fs = require('fs');
var request = require('request');

// global variables
var g_datadir = path.join(__dirname, 'data');
var g_nodes = [];
var g_logger = null;
var g_file_number = 0;
var g_file = null;
var g_client = null;
var g_client_num = 0;
var g_is_queried = false;
var QUERY_INTERVAL = 10;

module.exports = {
  start: start,
  addNode: addNode,
  setLogger: setLogger,
  onClientConnect: onClientConnect,
  onClientDisconnect: onClientDisconnect,
  getStatsOfDay: getStatsOfDay,
  getNodeList: getNodeList,
  getLogFileNumber: getLogFileNumber,
}

function StatInfo(id, addr, url, time, index, delay, info) {
  this.node_id = id;
  this.node_addr = addr;
  this.stats_url = url;
  this.time = time;
  this.m_index = index;
  this.delay = delay;
  this.info = info;
}

function Node(id, addr) {
  this.id = id;
  this.addr = addr;
  this.stats_url = addr + '/stats';
  if (addr.indexOf('http') == -1) {
    this.stats_url = 'http://' + this.stats_url;
  }
  this.alive = true;
}

function addNode(id, addr) {
  g_nodes[id] = new Node(id, addr);
}

function fireEvent(e, data) {
  if (g_client != null) {
    g_client.emit(e, data);
    g_client.broadcast.emit(e, data);
  }
}

function queryNode(start_time, index, node) {
  request(node.stats_url, function(error, response, body) {
    if (error || response.statusCode != 200) {
      g_logger.error('failed to query node: ' + node.stats_url_);
      if (node.alive_) {
        fireEvent('server status', {id: node.id, status: 'dead'});
        node.alive = false;
      }
      return;
    }
    var end_time = new Date();
    var delay = end_time - start_time;
    try {
      var json_info = JSON.parse(body);
      var stats_item = new StatInfo(
          node.id,
          node.addr,
          node.stats_url,
          start_time,
          index,
          delay,
          json_info.result);
      fireEvent('new stats', stats_item);
      if (!node.alive_) {
        fireEvent('server status', {id: node.id, status: 'alive'});
        node.alive = true;
      }
      fs.appendFile(g_file, JSON.stringify(stats_item) + '\n', function(err) {
        if (err) {
          g_logger.error('failed to append to file ' + g_file + ' ' + err);
        }
      });
    } catch (e) {
      g_logger.error('exception caught: ' + e);
    }
  });
}

function queryNodes() {
  var start_time = new Date();
  if (start_time.getSeconds() != 59) {
    g_is_queried = false;
    return;
  } else if (g_is_queried) {
    return;
  }
  var index = start_time.getHours() * 60 + start_time.getMinutes();
  if (index == 0) {
    // the new day
    g_file = getFileName();
    g_file_number++;
  }
  for (var i in g_nodes) {
    queryNode(start_time, index, g_nodes[i]);
  }
  g_is_queried = true;
}

function getFileName(date) {
  if (!date) {
    date = new Date();
  }
  var filename = util.format('%d_%d_%d.log',
      date.getFullYear(),
      date.getMonth() + 1,
      date.getDate());
  return path.join(__dirname, 'data', filename);
}

function setLogger(logger) {
  g_logger = logger;
}

function onClientConnect(client) {
  g_client_num++;
  g_client = client;
}

function onClientDisconnect(client) {
  g_client_num--;
  if (g_client_num == 0) {
    g_client = null;
  }
}

function accumulateObj(dst, src) {
  for (var i in src) {
    var type = typeof(src[i]);
    if (type == 'number') {
      if (!dst[i]) {
        dst[i] = 0;
      }
      dst[i] += src[i];
    } else if (type == 'object') {
      if (!dst[i]) {
        dst[i] = {};
      }
      accumulateObj(dst[i], src[i]);
    } else if (type == 'array') {
      if (!dst[i]) {
        dst[i] = [];
      }
      accumulateObj(dst[i], src[i]);
    }
  }
}

function getMinuteOfTime(time) {
  var d = new Date(time);
  var hour = d.getHours();
  hour = hour >= 10 ? hour : '0' + hour;
  var min = d.getMinutes();
  min = min >= 10 ? min : '0' + min;
  return hour + ':' + min;
}

function cloneObjTemplate(src) {
  var dst = {};
  for (var key in src) {
    var type = typeof(src[key]);
    if (type == 'object' || type == 'array') {
      dst[key] = cloneObjTemplate(src[key]);
    } else if (type == 'number') {
      dst[key] = 0;
    } else if (type == 'string') {
      dst[key] = '';
    } else if (type == 'boolean') {
      dst[key] = false;
    } else {
      g_logger.error('failed to clone type: ' + type);
    }
  }
  return dst;
}

function getStatsOfDay(option, cb) {
  g_logger.info('getStatsOfDay option: ' + util.inspect(option));
  var offset = option.offset;
  var node_id = option.id;
  var date = new Date();
  date.setDate(date.getDate() + option.offset);
  var filename = getFileName(date);
  g_logger.info('read stats log file: ' + filename);
  fs.readFile(filename, 'utf8', function(err, data) {
    if (err) {
      g_logger.error('failed to read stats log: ' + err);
      return cb([]);
    }
    var stats_list = [];
    var merged_stats = {};
    var one_valid_item = null;
    var lines = data.split('\n');
    for (var i in lines) {
      var line = lines[i];
      if (!line || line == '') {
        continue;
      }
      try {
        var item = JSON.parse(line);
        if (!one_valid_item) {
          one_valid_item = item;
        }
        if (node_id != -1) {
          if (item.node_id == node_id) {
            stats_list[item.m_index] = item;
          }
          continue;
        }
        if (merged_stats.m_index != undefined) {
          if (merged_stats.m_index == item.m_index) {
            accumulateObj(merged_stats.info, item.info);
          } else {
            stats_list[merged_stats.m_index] = merged_stats;
            merged_stats = item;
          }
        } else {
          merged_stats = item;
        }
      } catch (e) {
        g_logger.error('failed to parse stats log: ' + e + '\n' + line);
      }
    }
    if (merged_stats.m_index && stats_list.length > 0 &&
        merged_stats.m_index != stats_list[stats_list.length-1].m_index) {
      stats_list.push(merged_stats);
    }
    for (var i =0; i < stats_list.length; ++i) {
      if (!stats_list[i]) {
        stats_list[i] = {};
        stats_list[i].node_id = node_id;
        stats_list[i].node_addr = '--';
        stats_list[i].stats_url = '--';
        stats_list[i].time = '--';
        stats_list[i].minute = '--';
        stats_list[i].m_index = i;
        stats_list[i].delay = '--';
        stats_list[i].info = cloneObjTemplate(one_valid_item.info);
      } else {
        stats_list[i].minute = getMinuteOfTime(stats_list[i].time);
      }
    }
    cb(stats_list);
  });
}

function getNodeList() {
  return g_nodes;
}

function getLogFileNumber() {
  return g_file_number;
}

function start() {
  if (!fs.existsSync(g_datadir)) {
    fs.mkdirSync(g_datadir);
  }
  g_file_number = fs.readdirSync(g_datadir).length;
  g_file = getFileName();
  setInterval(queryNodes, QUERY_INTERVAL);
}
