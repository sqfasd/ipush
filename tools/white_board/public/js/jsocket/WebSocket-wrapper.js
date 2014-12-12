/* WebSocket-wrapper.js
 *
 * The MIT License
 *
 * Copyright (c) 2010 Tjeerd Jan 'Aidamina' van der Molen <aidamina@gmail.com>
 * http://jsocket.googlecode.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// parseUri 1.2.2
// (c) Steven Levithan <stevenlevithan.com>
// MIT License
function parseUri (str) {
	var	o   = parseUri.options,
		m   = o.parser[o.strictMode ? "strict" : "loose"].exec(str),
		uri = {},
		i   = 14;

	while (i--) uri[o.key[i]] = m[i] || "";

	uri[o.q.name] = {};
	uri[o.key[12]].replace(o.q.parser, function ($0, $1, $2) {
		if ($1) uri[o.q.name][$1] = $2;
	});

	return uri;
};

parseUri.options = {
	strictMode: false,
	key: ["source","protocol","authority","userInfo","user","password","host","port","relative","path","directory","file","query","anchor"],
	q:   {
		name:   "queryKey",
		parser: /(?:^|&)([^&=]*)=?([^&]*)/g
	},
	parser: {
		strict: /^(?:([^:\/?#]+):)?(?:\/\/((?:(([^:@]*)(?::([^:@]*))?)?@)?([^:\/?#]*)(?::(\d*))?))?((((?:[^?#\/]*\/)*)([^?#]*))(?:\?([^#]*))?(?:#(.*))?)/,
		loose:  /^(?:(?![^:@]+:[^:@\/]*@)([^:\/?#.]+):)?(?:\/\/)?((?:(([^:@]*)(?::([^:@]*))?)?@)?([^:\/?#]*)(?::(\d*))?)(((\/(?:[^?#](?![^?#\/]*\.[^?#\/.]+(?:[?#]|$)))*\/?)?([^?#\/]*))(?:\?([^#]*))?(?:#(.*))?)/
	}
};


(function(){

console = window.console || {log:function(){}};

function WebSocket(url,protocol){
	var ws = this;	
	var uri = parseUri(url);
	
	// TODO: some sanity/security checks on the uri and protocol
	
	this._eventEl = document.createElement("div");
	this._flashEl = document.createElement("div");
	
	function SendRequest(){
		var r = 
		"GET "+uri["path"]+" HTTP/1.1\n"+
		"Upgrade: WebSocket\n"+
		"Connection: Upgrade\n"+
		"Host: "+uri["host"]+":"+uri["port"]+"\n"+
		"Origin: "+window.location.protocol+"://"+window.location.host+"\n";		
		if(protocol)
			r+="WebSocket-Protocol: "+protocol+"\n";		
		r+="\n";
		
		ws._socket.write(r);
	
	}
	
	function ondata(size){
		
		if(ws.readyState==WebSocket.CONNECTING)
		{
			//TODO: Parse and validate handshake
			var handshake = ws._socket.readUTFBytes(size);
			
			
			var event = document.createEvent("Event");
			event.initEvent("open",false,false);
			ws.dispatchEvent(event);
		
			ws.readyState=WebSocket.OPEN;
			
		}
		else if(ws.readyState==WebSocket.OPEN)
		{
			var intro = ws._socket.readByte();
			var data = ws._socket.readUTFBytes(size-2);
			var outro = ws._socket.readByte();
			var event = document.createEvent("Event");
			event.initEvent("message",false,false);
			event.data = data;
			ws.dispatchEvent(event);
			
		}
	}
	function onclose(){
	
		var event = document.createEvent("Event");
		event.initEvent("close",false,false);
		ws.dispatchEvent(event);
	
	}
	
	function onready(){	
		ws._socket.connect(uri["host"],uri["port"]);
	
	}
	function onconnect(result,msg){
	
		if(result){
			SendRequest();
		} else {
			var event = document.createEvent("Event");
			event.initEvent("close",false,false);

			ws.dispatchEvent(event);
		}
	}
	
	this._socket = new jSocket(onready, onconnect, ondata, onclose);
	function rdy(){ return document.readyState === "complete"; }
	
	function setup(){
		var tar = "__"+ws._socket.id;
		ws._flashEl.setAttribute("id",tar);
		document.body.appendChild(ws._flashEl);	
		ws._socket.setup(tar);
	}
	
	if ( rdy() ) 
		setup();
	else{
		var iv = window.setInterval(function(){
			if ( rdy() ) {
				window.clearInterval(iv);
				setup();			
			}		
		},50);
	}
}
WebSocket.CONNECTING  =0;
WebSocket.OPEN = 1;
WebSocket.CLOSING = 2;
WebSocket.CLOSED = 3;

WebSocket.prototype.readyState = WebSocket.CONNECTING;
WebSocket.prototype.bufferedAmount = 0;

WebSocket.prototype.send = function(data){
		
		this._socket.writeByte(0x00);
		this._socket.writeUTFBytes(data);
		this._socket.writeByte(0xFF);
		this._socket.flush();
}


WebSocket.prototype.close = function(){
	
	this.readyState = WebSocket.CLOSING;
	this._socket.flush();
	this._socket.close();

}

WebSocket.prototype.addEventListener = function(type,listener,useCapture){
	this._eventEl.addEventListener(type,listener,useCapture);
}
WebSocket.prototype.removeEventListener = function(type,listener,useCapture){
	this._eventEl.removeEventListener(type,listener,useCapture);
}
WebSocket.prototype.dispatchEvent = function(evt){
	console.log("dispatched event "+evt.type);
	if(["open","message","error","close"].indexOf(evt.type)!=-1)
	{
		var h = this["on"+evt.type];
		if(h)
			h(evt);
	
	}
	if(evt.type==="close")
		this.readyState = WebSocket.CLOSED;
	return this._eventEl.dispatchEvent(evt);

}
if (!"WebSocket" in window)
	window.WebSocket = WebSocket;


})();