#include "src/websocket/websocket.h"

#include "src/crypto/base64.h"
#include "src/crypto/sha1.h"

#include <arpa/inet.h>
#include <string.h>
#include <iostream>

using namespace std;

WebSocket::WebSocket() {
}

WebSocketFrameType WebSocket::parseHandshake(unsigned char* input_frame,
                                             int input_len) {
  // 1. copy char*/len into string
  // 2. try to parse headers until \r\n occurs
  string headers((char*)input_frame, input_len);
  int header_end = headers.find("\r\n\r\n");

  if (header_end == string::npos) { // end-of-headers not found - do not parse
    return INCOMPLETE_FRAME;
  }

  headers.resize(header_end); // trim off any data we don't need after the headers
  vector<string> headers_rows = explode(headers, string("\r\n"));
  for(int i=0; i<headers_rows.size(); i++) {
    string& header = headers_rows[i];
    if (header.find("GET") == 0) {
      vector<string> get_tokens = explode(header, string(" "));
      if (get_tokens.size() >= 2) {
        this->resource = get_tokens[1];
      }
    } else {
      int pos = header.find(":");
      if (pos != string::npos) {
        string header_key(header, 0, pos);
        string header_value(header, pos+1);
        header_value = trim(header_value);
        if (header_key == "Host") {
          this->host = header_value;
        } else if (header_key == "Origin") {
          this->origin = header_value;
        } else if (header_key == "Sec-WebSocket-Key") {
          this->key = header_value;
        } else if (header_key == "Sec-WebSocket-Protocol") {
          this->protocol = header_value;
        }
      }
    }
  }

  //this->key = "dGhlIHNhbXBsZSBub25jZQ==";
  //printf("PARSED_KEY:%s \n", this->key.data());

  //return FrameType::OPENING_FRAME;
  printf("HANDSHAKE-PARSED\n");
  return OPENING_FRAME;
}

string WebSocket::trim(string str) {
  //printf("TRIM\n");
  static const char* whitespace = " \t\r\n";
  string::size_type pos = str.find_last_not_of(whitespace);
  if (pos != string::npos) {
    str.erase(pos + 1);
    pos = str.find_first_not_of(whitespace);
    if (pos != string::npos) str.erase(0, pos);
  }
  else {
    return string();
  }
  return str;
}

vector<string> WebSocket::explode(string theString,
                                  string theDelimiter,
                                  bool theIncludeEmptyStrings) {
  //printf("EXPLODE\n");
  //UASSERT( theDelimiter.size(), >, 0 );

  vector<string> theStringVector;
  int  start = 0, end = 0, length = 0;

  while ( end != string::npos )
  {
    end = theString.find( theDelimiter, start );

    // If at end, use length=maxLength.  Else use length=end-start.
    length = (end == string::npos) ? string::npos : end - start;

    if (theIncludeEmptyStrings
        || (   ( length > 0 ) /* At end, end == length == string::npos */
          && ( start  < theString.size() ) ) )
      theStringVector.push_back( theString.substr( start, length ) );

    // If at end, use start=maxSize.  Else use start=end+delimiter.
    start = (   ( end > (string::npos - theDelimiter.size()) )
        ?  string::npos  :  end + theDelimiter.size()     );
  }
  return theStringVector;
}

string WebSocket::answerHandshake() {
  unsigned char digest[20]; // 160 bit sha1 digest

  string answer;
  answer += "HTTP/1.1 101 Switching Protocols\r\n";
  answer += "Upgrade: WebSocket\r\n";
  answer += "Connection: Upgrade\r\n";
  if (this->key.length() > 0) {
    string accept_key;
    accept_key += this->key;
    accept_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; //RFC6544_MAGIC_KEY

    //printf("INTERMEDIATE_KEY:(%s)\n", accept_key.data());

    SHA1 sha;
    sha.Input(accept_key.data(), accept_key.size());
    sha.Result((unsigned*)digest);

    //printf("DIGEST:"); for(int i=0; i<20; i++) printf("%02x ",digest[i]); printf("\n");

    //little endian to big endian
    for(int i=0; i<20; i+=4) {
      unsigned char c;

      c = digest[i];
      digest[i] = digest[i+3];
      digest[i+3] = c;

      c = digest[i+1];
      digest[i+1] = digest[i+2];
      digest[i+2] = c;
    }

    //printf("DIGEST:"); for(int i=0; i<20; i++) printf("%02x ",digest[i]); printf("\n");

    accept_key = Base64Encode((const unsigned char *)digest, 20); //160bit = 20 bytes/chars

    answer += "Sec-WebSocket-Accept: "+(accept_key)+"\r\n";
  }
  if (this->protocol.length() > 0) {
    answer += "Sec-WebSocket-Protocol: "+(this->protocol)+"\r\n";
  }
  answer += "\r\n";
  return answer;

  //return WS_OPENING_FRAME;
}

string WebSocket::getAcceptKey() {
  unsigned char digest[20]; // 160 bit sha1 digest
  string accept_key = this->key;
  accept_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; //RFC6544_MAGIC_KEY

  SHA1 sha;
  sha.Input(accept_key.data(), accept_key.size());
  sha.Result((unsigned*)digest);
  for(int i=0; i<20; i+=4) {
    unsigned char c;

    c = digest[i];
    digest[i] = digest[i+3];
    digest[i+3] = c;

    c = digest[i+1];
    digest[i+1] = digest[i+2];
    digest[i+2] = c;
  }
  accept_key = Base64Encode((const unsigned char *)digest, 20);
  return accept_key;
}

string WebSocket::getProtocol() {
  return this->protocol;
}

int WebSocket::makeFrame(WebSocketFrameType frame_type,
                         unsigned char* msg,
                         int msg_length,
                         unsigned char* buffer,
                         int buffer_size) {
  int pos = 0;
  int size = msg_length;
  buffer[pos++] = (unsigned char)frame_type; // text frame

  if (size<=125) {
    buffer[pos++] = size;
  }
  else if (size<=65535) {
    buffer[pos++] = 126; //16 bit length
    buffer[pos++] = (size >> 8) & 0xFF; // rightmost first
    buffer[pos++] = size & 0xFF;
  }
  else { // >2^16-1
    buffer[pos++] = 127; //64 bit length

    //TODO: write 8 bytes length
    pos+=8;
  }
  memcpy((void*)(buffer+pos), msg, size);
  return (size+pos);
}

WebSocketFrameType WebSocket::getFrame(unsigned char* in_buffer,
                                       int in_length,
                                       unsigned char* out_buffer,
                                       int out_size,
                                       int* out_length) {
  //printf("getTextFrame()\n");
  if (in_length < 3) {
    return INCOMPLETE_FRAME;
  }

  unsigned char msg_opcode = in_buffer[0] & 0x0F;
  unsigned char msg_fin = (in_buffer[0] >> 7) & 0x01;
  unsigned char msg_masked = (in_buffer[1] >> 7) & 0x01;

  // *** message decoding

  int payload_length = 0;
  int pos = 2;
  int length_field = in_buffer[1] & (~0x80);
  unsigned int mask = 0;

  //printf("IN:"); for(int i=0; i<20; i++) printf("%02x ",buffer[i]); printf("\n");

  if (length_field <= 125) {
    payload_length = length_field;
  }
  else if (length_field == 126) { //msglen is 16bit!
    payload_length = ntohs(*(uint16_t*)(in_buffer+2));
    pos += 2;
  }
  else if (length_field == 127) { //msglen is 64bit!
    payload_length = ntohs(*(uint64_t*)(in_buffer+2));
    pos += 8;
  }
  //printf("PAYLOAD_LEN: %08x, length_field: %d\n", payload_length, length_field);
  if (in_length < payload_length+pos) {
    return INCOMPLETE_FRAME;
  }

  if (msg_masked) {
    mask = *((unsigned int*)(in_buffer+pos));
    //printf("MASK: %08x\n", mask);
    pos += 4;

    // unmask data:
    unsigned char* c = in_buffer+pos;
    for(int i=0; i<payload_length; i++) {
      c[i] = c[i] ^ ((unsigned char*)(&mask))[i%4];
    }
  }

  if (payload_length > out_size) {
    //TODO: if output buffer is too small -- ERROR or resize(free and allocate bigger one) the buffer ?
  }

  memcpy((void*)out_buffer, (void*)(in_buffer+pos), payload_length);
  out_buffer[payload_length] = 0;
  *out_length = payload_length+1;

  //printf("TEXT: %s\n", out_buffer);

  if (msg_opcode == 0x0) {
    return (msg_fin)?TEXT_FRAME:INCOMPLETE_TEXT_FRAME; // continuation frame ?
  }
  if (msg_opcode == 0x1) {
    return (msg_fin)?TEXT_FRAME:INCOMPLETE_TEXT_FRAME;
  }
  if (msg_opcode == 0x2) {
    return (msg_fin)?BINARY_FRAME:INCOMPLETE_BINARY_FRAME;
  }
  if (msg_opcode == 0x9) {
    return PING_FRAME;
  }
  if (msg_opcode == 0xA) {
    return PONG_FRAME;
  }
  return ERROR_FRAME;
}
