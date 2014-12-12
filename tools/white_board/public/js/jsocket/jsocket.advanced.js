/* jSocket.advanced.as
 *
 * The MIT License
 *
 * Copyright (c) 2008 Tjeerd Jan 'Aidamina' van der Molen <aidamina@gmail.com>
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

/**
 * String defining the default swf file
 * overrides jSocket.swf in jsocket.js
 * @var String
 */
jSocket.swf = "jsocket.advanced.swf";

/**
 * Flushes the write buffer to the remote host
 *
 * You need to use this function everytime you want to
 * send data to the server when using the advanced socket
 */
jSocket.prototype.flush = function() {
  this.assertConnected();
  return this.movie.flush();
}

/**
 * Write the contents of the array one by one to the write buffer
 * The SWF figures out what type of write to use
 * @param array {data}
 */
jSocket.prototype.writeArray = function(data) {
  this.assertConnected();
  this.movie.writeArray(data);
}

/**
 * Write a Boolean to the write buffer
 * @param bool {data}
 */
jSocket.prototype.writeBoolean = function(data) {
  this.assertConnected();
  this.movie.writeBoolean(data);
}

/**
 * Read a Boolean from the read buffer
 * @return bool
 */
jSocket.prototype.readBoolean = function() {
  this.assertConnected();
  return this.movie.readBoolean();
}

/**
 * Write a single Byte to the write buffer
 * @param String {data}
 */
jSocket.prototype.writeByte = function(data) {
  this.assertConnected();
  this.movie.writeByte(data);
}

/**
 * Read a single Byte from the read buffer
 * @return bool
 */
jSocket.prototype.readByte = function() {
  this.assertConnected();
  return this.movie.readByte();
}

/**
 * Write a sequence of Bytes to the write buffer
 * @param Array {data} Array holding the bytes to write
 * @param int {offset} (optional)
 * @param int {length} (optional)
 */
jSocket.prototype.writeBytes = function(bytes, offset, length){
  this.assertConnected();
  this.movie.writeBytes(bytes, offset, length);
}

/**
 * Read a sequence of Bytes from the read buffer
 * @param int {offset} (optional)
 * @param int {length} (optional)
 * @return Array
 */
jSocket.prototype.readBytes = function(offset, length) {
  this.assertConnected();
  return this.movie.readBytes(offset, length);
}

/**
 * Write a short 16-bit integer to the write buffer
 * @param int {data} 16-bit integer
 */
jSocket.prototype.writeShort = function(data) {
  this.assertConnected();
  this.movie.writeShort(data);
}

/**
 * Read a short 16-bit integer from the read buffer
 * @return int 16-bit integer
 */
jSocket.prototype.readShort = function() {
  this.assertConnected();
  return this.movie.readShort();
}

/**
 * Write a signed integer to the write buffer
 * @param int {data}
 */
jSocket.prototype.writeInt = function(data) {
  this.assertConnected();
  this.movie.writeInt(data);
}

/**
 * Read a signed integer from the read buffer
 * @return int
 */
jSocket.prototype.readInt = function() {
  this.assertConnected();
  return this.movie.readInt();
}

/**
 * Write a unsigned integer to the write buffer
 * @param int {data}
 */
jSocket.prototype.writeUnsignedInt = function(data) {
  this.assertConnected();
  this.movie.writeUnsignedInt(data);
}

/**
 * Read a unsigned integer from the read buffer
 * @return int
 */
jSocket.prototype.readUnsignedInt = function() {
  this.assertConnected();
  return this.movie.readUnsignedInt();
}

/**
 * Write a float to the write buffer
 * @param float {data}
 */
jSocket.prototype.writeFloat = function(data) {
  this.assertConnected();
  this.movie.writeFloat(data);
}

/**
 * Read a float from the read buffer
 * @return float
 */
jSocket.prototype.readFloat = function() {
  this.assertConnected();
  return this.movie.readFloat();
}

/**
 * Write a double to the write buffer
 * @param float {data}
 */
jSocket.prototype.writeDouble = function(data) {
  this.assertConnected();
  this.movie.writeDouble(data);
}

/**
 * Read a double from the read buffer
 * @return float
 */
jSocket.prototype.readDouble = function() {
  this.assertConnected();
  return this.movie.readDouble();
}

/**
 * Write a multiByte string to the write buffer
 * @param string {data} The string to send
 * @param string {charSet} The charset of the string that is being send (valid charset codes: http://help.adobe.com/en_US/AS3LCR/Flash_10.0/charset-codes.html)
 */
jSocket.prototype.writeMultiByte = function(data, charSet) {
  this.assertConnected();
  this.movie.writeMultiByte(data, charSet);
}

/**
 * Read a multiByte string from the read buffer
 * @param int {length} The number of bytes to read from the read buffer
 * @param string {charSet} The string denoting the character set to use to interpret the bytes.
 * @return string The string is always returned in UTF-8
 */
jSocket.prototype.readMultiByte = function(length,charSet) {
  this.assertConnected();
  return this.movie.readMultiByte(length, charSet);
}

/**
 * Write a UTF-8 encoded string to the write buffer
 * @param string {data} The string to write
 */
jSocket.prototype.writeUTFBytes = function(data) {
  this.assertConnected();
  this.movie.writeUTFBytes(data);
}

/**
 * Reads the number of UTF-8 data bytes specified by the length parameter from the socket, and returns a string.
 * @param int {length} The number of bytes to read.
 * @return string A UTF-8 string
 */
jSocket.prototype.readUTFBytes = function(length) {
  this.assertConnected();
  return this.movie.readUTFBytes(length);
}

/**
 * Writes the following data to the write buffer: a 16-bit unsigned integer, which indicates the length of the specified UTF-8 string in bytes, followed by the string itself.
 * @param string {data} The string to write
 */
jSocket.prototype.writeUTF = function(data) {
  this.assertConnected();
  this.movie.writeUTF(data);
}

/**
 * Reads a UTF-8 string from the read buffer. The string is assumed to be prefixed with an unsigned short integer that indicates the length in bytes.
 * @return string A UTF-8 string
 */
jSocket.prototype.readUTF = function() {
  this.assertConnected();
  return this.movie.readUTF();
}

/**
 * Write an object to the write buffer in AMF serialized format.
 * @see jSocket.prototype.getObjectEncoding
 * @param object {data} The object to be serialized
 */
jSocket.prototype.writeObject = function(data) {
  this.assertConnected();
  this.movie.writeObject(data);
}

/**
 * Reads an object from the socket, encoded in AMF serialized format.
 * @see jSocket.prototype.getObjectEncoding
 * @return object The deserialized object
 */
jSocket.prototype.readObject = function() {
  this.assertConnected();
  return this.movie.readObject();
}

/**
 * Sets the object encoding when serializing objects
 * @param int {value} 0 for AS 1.0 and 2.0. 3 for AS 3.0
 */
jSocket.prototype.setObjectEncoding = function(value) {
  this.assertConnected();
  this.movie.setObjectEncoding(value);
}

/**
 * Gets the object encoding
 * @return int 0 for AS 1.0 and 2.0. 3 for AS 3.0
 */
jSocket.prototype.getObjectEncoding = function() {
  this.assertConnected();
  return this.movie.getObjectEncoding();
}

/**
 * Set the byte order for the data (default is bigEndian)
 * @param string {value} either "bigEndian" or "littleEndian"
 */
jSocket.prototype.setEndian = function(value) {
  this.assertConnected();
  this.movie.setEndian(value);
}

/**
 * Get the byte order used for the data
 * @return string either "bigEndian" or "littleEndian"
 */
jSocket.prototype.getEndian = function() {
  this.assertConnected();
  return this.movie.getEndian();
}

/**
 * The number of bytes available for reading
 * @return int
 */
jSocket.prototype.getBytesAvailable = function() {
  this.assertConnected();
  return this.movie.getBytesAvailable();
}
