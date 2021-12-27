#ifndef MSG_PROTOCOL_H
#define MSG_PROTOCOL_H

#include <iostream>
#include <fstream>
#include <string.h>
#include <map>
#include <vector>
#include "cpp-bencoding/src/bencoding.h"

#define MAX_BLOCK 16384
using namespace std;

class MSG{
public:
  int lenPayload, msgType, index, begin, blockLen, reqLen;
  uint8_t hash[20];
  uint8_t peerId[20];
  uint8_t block[MAX_BLOCK];
  
 public:
  /*Return message type*/
  MSG decode(uint8_t *msg, size_t msgLength);
  
  
  
  /*encode messages given proper input*/
  void handshake(uint8_t *msg, size_t *msgByte,uint8_t hash[20], uint8_t peerId[20]);
  void keepAlive(uint8_t *msg, size_t *msgByte);
  void choke(uint8_t *msg, size_t *msgByte);
  void unchoke(uint8_t *msg, size_t *msgByte);
  void interested(uint8_t *msg, size_t *msgByte);
  void notInterested(uint8_t *msg, size_t *msgByte);
  void have(uint8_t *msg, size_t *msgByte, int index);
  void request(uint8_t *msg, size_t *msgByte, int index, int begin, int length);
  void bitfield(uint8_t *msg, size_t *msgByte);
  void piece(uint8_t *msg, size_t *msgByte, int index, int begin, uint8_t *block, int blockLen);
  void cancel(uint8_t *msg, size_t *msgByte, int index, int begin, int length);



};
#endif
