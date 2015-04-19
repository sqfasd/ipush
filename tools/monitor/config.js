var path = require('path');
var fs = require('fs');
var winston = require('winston');

var node_env = process.env.NODE_ENV;

function log_transports() {
  if (node_env == 'development') {
    var options = {
      timestamp: true,
      level: 'debug',
      json: false,
    };
    return [new winston.transports.Console(options)];
  } else {
    var options = {
      handleExceptions: true,
      timestamp: true,
      level: 'info',
      json: false,
      filename: path.join(__dirname, 'logs', 'monitor.log'),
    };
    return [new winston.transports.DailyRotateFile(transport_options)];
  }
}

function create_logger() {
  var logdir = path.join(__dirname, 'logs');
  if (!fs.existsSync(logdir)) {
    fs.mkdirSync(logdir);
  }
  return new winston.Logger({
    transports: log_transports(),
    exitOnError: false,
  });
}

if (node_env == 'development') {
  module.exports = {
    logger: create_logger(),
    nodes: [
      "localhost:9001",
      "localhost:9003",
    ],
  };
} else if (node_env == 'production') {
  module.exports = {
    logger: create_logger(),
    nodes: [
      "localhost:9001",
      "localhost:9003",
    ],
  };
} else {
  require('assert')(false, 'unexpected NODE_ENV: ' + node_env);
}
