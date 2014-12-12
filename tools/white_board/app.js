var net = require('net');
var fs = require('fs');
var path = require('path');
var express = require('express');
var bodyParser = require('body-parser');
var ejs = require('ejs');
var config = require('./config');

var app = module.exports = express();

app.use(express.static(path.join(__dirname, 'public')));
app.use(express.static(path.join(__dirname, 'upload')));
app.use(bodyParser({keepExtensions: true, uploadDir: '/tmp'}));

app.engine('.html', ejs.__express);
app.set('view engine', 'html');
app.set('views', __dirname + '/views');

app.get('/', function(req, res) {
  res.render('index', config.default_push_server);
});
app.post('/upload_image', function(req, res) {
  var name = req.body.name;
  var full_path = path.join('upload', name);
  var image_base64 = req.body.image_data;
  console.log(image_base64);
  var data_offset = "data:image/png;base64,".length;
  var image_data= new Buffer(image_base64.substring(data_offset), 'base64');
  fs.writeFile(full_path, image_data, function(err) {
    if (err) {
      res.json({error: err});
    } else {
      res.json({image_uri: '/' + name});
    }
  });
});

var security_policy_server = net.createServer(function(conn) {
  console.log('security request come');
  conn.on('data', function(data) {
    var utf8_str = data.toString('utf8');
    if (utf8_str.indexOf('<policy-file-request/>') >= 0) {
      conn.end('<?xml version="1.0" encoding="UTF-8"?>' +
               '<cross-domain-policy>' +
               '<site-control permitted-cross-domain-policies="all" />' +
               '<allow-access-from domain="*" to-ports="*" secure="false" />' +
               '</cross-domain-policy>');
    } else {
      console.warn('unkonwn request: ' + utf8_str);
      conn.end('unknow request');
    }
  });
});

if (!module.parent) {
  app.listen(3100);
  security_policy_server.listen(843);
  console.log('Express started on port 3100');
  console.log('run mode: ' + process.env.NODE_ENV);
}
