var path = require('path');
var util = require('util');
var fs = require('fs');
var http = require('http');
var express = require('express');
var socketio = require('socket.io');
var ejs = require('ejs');
var monitor = require('./monitor');
var config = require('./config');
var logger = config.logger;

monitor.setLogger(logger);
for (var i in config.nodes) {
  monitor.addNode(i, config.nodes[i]);
}
monitor.start();

var app = express();
var server = http.createServer(app);
var io = socketio(server);

app.use(express.static(path.join(__dirname, 'public')));

app.engine('.html', ejs.__express);
app.set('view engine', 'html');
app.set('views', path.join(__dirname, 'views'));

function getDay(offset) {
  var d = new Date();
  d.setDate(d.getDate() + offset);
  return util.format('%d-%d-%d',
      d.getFullYear(), d.getMonth() + 1, d.getDate());
}

app.get('/', function(req, res, next) {
  logger.info('request: ' + req.url);
  monitor.getStatsOfDay({offset: 0, id: -1}, function(stats_list) {
    logger.info('getStatsOfDay stats_list.length=' + stats_list.length);
    res.render('index', {
      node_list: monitor.getNodeList(),
      stats_list: stats_list
    });
  });
});

app.get('/history', function(req, res, next) {
  logger.info('request: ' + req.url);
  var offset = Number(req.query.offset || -1);
  var node_id = Number(req.query.node_id || -1);
  if (node_id < -1 || node_id >= config.nodes.length) {
    res.send('Bad Request: invalid node id ' + node_id);
    return;
  }
  var history_file_number = monitor.getLogFileNumber() - 1;
  var date = getDay(offset);
  monitor.getStatsOfDay({offset: offset, id: -1}, function(stats_list) {
    res.render('history', {
      history_file_number: history_file_number,
      stats_list: stats_list,
      offset: offset,
      date: date,
      node_id: node_id,
    });
  });
});

app.get('/chart', function(req, res, next) {
  logger.info('request: ' + req.url);
  var offset = Number(req.query.offset || 0);
  var node_id = Number(req.query.node_id || -1);
  if (node_id < -1 || node_id >= config.nodes.length) {
    res.send('Bad Request: invalid node id ' + node_id);
    return;
  }
  var stats_file_number = monitor.getLogFileNumber();
  var date = getDay(offset);
  res.render('chart', {
    stats_file_number: stats_file_number,
    offset: offset,
    date: date,
    node_id: node_id,
  });
});

app.get('/get_stats_log', function(req, res, next) {
  logger.info('request: ' + req.url);
  var offset = Number(req.query.offset || 0);
  var node_id = Number(req.query.node_id || -1);
  if (node_id < -1 || node_id >= config.nodes.length) {
    res.send({error: 'Bad Request: invalid node id ' + node_id});
    return;
  }
  monitor.getStatsOfDay({offset: offset, id: node_id}, function(stats_list) {
    res.send({error: null, result: stats_list});
  });
});

app.get('/node', function(req, res, next) {
  logger.info('request: ' + req.url);
  var node_id = Number(req.query.id || -1);
  if (node_id <= -1 || node_id >= config.nodes.length) {
    res.send('Bad Request: invalid node id ' + node_id);
    return;
  }
  monitor.getStatsOfDay({offset: 0, id: node_id}, function(stats_list) {
    res.render('node', {
      node_id: node_id,
      node_addr: config.nodes[node_id],
      stats_list: stats_list
    });
  });
});

io.on('connection', function(socket) {
  logger.info('new socket come');
  monitor.onClientConnect(socket);

  socket.on('disconnect', function() {
    logger.info('disconnect');
    monitor.onClientConnect(socket);
  });
});

var port = config.port || 8083;
server.listen(port, function() {
  logger.info('Express started on port ' + port);
  logger.info('run mode: ' + process.env.NODE_ENV);
});
