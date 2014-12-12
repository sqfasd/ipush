/* jSocket.js
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

;(function($){
    /**
    * Turn a DOMnode into a socket
    * Note: Replaces the node with the swf for the socket
    *
    * @param {options} {onReady} When the SWF is added the to document and ready for use
    * @param {options} {onConnect} Connection attempt finished (either succesfully or with an error)
    * @param {options} {onData} Socket received data from the remote host
    * @param {options} {onClose} Remote host disconnects the connection
    * @param {options} {swflocation} Location of the SWF file
    * @return jSocket
    */
  $.fn.socket = function(options) {
    if(1 != this.length) return;
    var socket = $.fn.socket.sockets[this.attr('id')];
    if(!socket) {
      options = $.extend(true, {}, $.fn.socket.defaults, options);
      socket = new jSocket(options.onReady, options.onConnect, options.onData, options.onClose);
      if(!this.attr('id'))
        this.attr('id', socket.id);
      var id = this.attr('id');
      socket.setup(id, options.swf);
      $.fn.socket.sockets[id]=socket;
    }
    return socket;
  };

  $.fn.socket.sockets = {};

  $.fn.socket.defaults = {
    onReady : false,
    onConnect : false,
    onData : false,
    onClose : false,
    swf : null
  };
})(jQuery);
