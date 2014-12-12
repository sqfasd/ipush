/* jSocket.as
 * 
 * The MIT License
 * 
 * Copyright (c) 2008 Tjeerd Jan 'Aidamina' van der Molen <aidamina@gmail.com>
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

package 
{
	import flash.display.Sprite;
	import flash.external.ExternalInterface;
	import flash.events.*;
	import flash.net.Socket;
	
	public class jSocket extends Sprite
	{		
		protected var socket:Socket;
		protected var id:String;
		
		public function jSocket():void {
			// Pass exceptions between flash and browser
			ExternalInterface.marshallExceptions = true;
			
			var url:String = root.loaderInfo.url;
			id = url.substring(url.lastIndexOf("?") + 1);
			
			socket = new Socket();
			socket.addEventListener("close", onClose);
			socket.addEventListener("connect", onConnect);
			socket.addEventListener("ioError", onError);
			socket.addEventListener("securityError", onSecurityError);
			socket.addEventListener("socketData", onData);
			
					
			ExternalInterface.addCallback("connect", connect);
			ExternalInterface.addCallback("close", close);
			ExternalInterface.addCallback("write", write);
			
			ExternalInterface.call("jSocket.flashCallback", "init", id);
		}
		
		public function connect(host:String, port:int):void{
			socket.connect(host, port);
		}
		
		public function close():void{
			socket.close();
		}
		
		public function write(object:*):void{
			socket.writeUTFBytes(object);
			socket.flush();
		}
		
		protected function onConnect(event:Event):void{
			ExternalInterface.call("jSocket.flashCallback", "connect", id);
		}
		
		protected function onError(event:IOErrorEvent):void{
			ExternalInterface.call("jSocket.flashCallback", "error", id, event.text);
		}
		
		protected function onSecurityError(event:SecurityErrorEvent):void{
			ExternalInterface.call("jSocket.flashCallback", "error", id, event.text);
		}
		
		protected function onClose(event:Event):void{
            socket.close();
			ExternalInterface.call("jSocket.flashCallback", "close", id);
		}
		
		protected function onData(event:ProgressEvent):void{
			ExternalInterface.call("jSocket.flashCallback", "data", id, socket.readUTFBytes(event.bytesLoaded));
		}
	}	
}

