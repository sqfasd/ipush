#ifndef SRC_WEBSOCKET_WEBSOCKET_H_
#define SRC_WEBSOCKET_WEBSOCKET_H_

#include <assert.h>
#include <stdint.h> /* uint8_t */
#include <stdio.h> /* sscanf */
#include <ctype.h> /* isdigit */
#include <stddef.h> /* int */

#include <vector> 
#include <string> 

enum WebSocketFrameType {
  ERROR_FRAME=0xFF00,
  INCOMPLETE_FRAME=0xFE00,

  OPENING_FRAME=0x3300,
  CLOSING_FRAME=0x3400,

  INCOMPLETE_TEXT_FRAME=0x01,
  INCOMPLETE_BINARY_FRAME=0x02,

  TEXT_FRAME=0x81,
  BINARY_FRAME=0x82,

  PING_FRAME=0x19,
  PONG_FRAME=0x1A
};

class WebSocket {
 public:
  std::string resource;
  std::string host;
  std::string origin;
  std::string protocol;
  std::string key;

  WebSocket();

  /**
   * @param input_frame .in. pointer to input frame
   * @param input_len .in. length of input frame
   * @return [WS_INCOMPLETE_FRAME, WS_ERROR_FRAME, WS_OPENING_FRAME]
   */
  WebSocketFrameType parseHandshake(unsigned char* input_frame, int input_len);
  std::string answerHandshake();
  std::string getAcceptKey();
  std::string getProtocol();

  int makeFrame(WebSocketFrameType frame_type,
                unsigned char* msg,
                int msg_len,
                unsigned char* buffer,
                int buffer_len);
  WebSocketFrameType getFrame(unsigned char* in_buffer,
                              int in_length,
                              unsigned char* out_buffer,
                              int out_size,
                              int* out_length);

  std::string trim(std::string str);
  std::vector<std::string> explode(std::string theString,
                                   std::string theDelimiter,
                                   bool theIncludeEmptyStrings = false );
};

#endif  /* WEBSOCKET_H */
