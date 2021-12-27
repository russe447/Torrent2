#include <iostream>
#include <fstream>
#include <string.h>
#include <map>
#include <bits/stdc++.h> 
#include <openssl/sha.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bencode/bencode.h"
#include "read_metafile.h"
#include "msg_protocol.h"


using namespace std;



MSG MSG::decode(uint8_t *msg, size_t msgLength){
  MSG retMsg;
  const char *handshake = "BitTorrent protocol";
    

  if (msg[0] == 19) {
    char chr[20];
    memcpy(chr, msg + 1, 19);
    
    
    if(strcmp(chr, handshake) != 0) {
      memcpy(retMsg.hash , msg+28,20);
      memcpy(retMsg.peerId, msg+48,20);
     
    }

   
  } else {

    retMsg.lenPayload = ntohl(*(uint32_t*)&msg[0]);
    

    if (retMsg.lenPayload == 0) {

    }else if (retMsg.lenPayload == 1) {
      retMsg.msgType = msg[4];
    }else if (retMsg.lenPayload == 5) {
      retMsg.msgType = msg[4];
    }else if (retMsg.lenPayload == 13) {
      retMsg.msgType = msg[4];
      retMsg.index = ntohl(*(uint32_t*) &msg[5]);
      retMsg.begin = ntohl(*(uint32_t*) &msg[9]);
      retMsg.reqLen = ntohl(*(uint32_t*) &msg[13]);
    }else {
      
      retMsg.msgType = msg[4];
      retMsg.index = msg[5];
      retMsg.begin = msg[9];
      if (retMsg.msgType == 5) {


      }else if (retMsg.msgType == 7) {
	
	bzero(retMsg.block,MAX_BLOCK);
	
	for (int i = 14; i < (int)msgLength; i++) {
	  retMsg.block[i-14] = msg[i];
	}
        retMsg.blockLen = msgLength - 14;  
	
      }
    }
    
  }

  
  return retMsg; 
}






/*Handshake is of length 70*/
void MSG::handshake(uint8_t *msg, size_t *msgByte,uint8_t *hash, uint8_t *peerId) {
  
  const char *handshake = "BitTorrent protocol";
  
  bzero(msg,68);
  //set header
  msg[0] = 19;
  memcpy(msg+1,handshake,strlen(handshake));
  //set hash
  memcpy(msg+28,hash,20);
  memcpy(msg+48, peerId, 20);
  *msgByte = 68;
  
}

void MSG::keepAlive(uint8_t *msg, size_t *msgByte) {

  bzero(msg,4);
  *msgByte = 4;
  
}


void MSG::choke(uint8_t *msg, size_t *msgByte) {

  bzero(msg, 5);
  (uint32_t&)*msg = htonl(1);
  *msgByte = 5;
  

}

void MSG::unchoke(uint8_t *msg, size_t *msgByte) {

  bzero(msg,5);
  (uint32_t&)*msg = htonl(1);
  msg[4] = 1; 
  *msgByte = 5;
  
  
}

void MSG::interested(uint8_t *msg, size_t *msgByte) {
  bzero(msg,5);
  (uint32_t&)*msg = htonl(1);
  msg[4] = 2; 
  *msgByte = 5;
 
}

void MSG::notInterested(uint8_t *msg, size_t *msgByte) {
  bzero(msg,5);
  (uint32_t&)*msg = htonl(1);
  msg[4] = 3; 
  *msgByte = 5;
 
}

void MSG::have(uint8_t *msg, size_t *msgByte, int index) {
  
  bzero(msg,9);
  (uint32_t&)*msg = htonl(5);
  msg[4] = 4;
  (uint32_t&)msg[5] = htonl(index);
  *msgByte = 9;
  

}

void MSG::request(uint8_t *msg, size_t *msgByte, int index, int begin, int length) {
  bzero(msg,13);
  (uint32_t&)*msg = htonl(13);
  msg[4] = 6;
  (uint32_t&)msg[5] = htonl(index);
  (uint32_t&)msg[9] = htonl(begin);
  (uint32_t&)msg[13] = htonl(length);
  *msgByte = 17;
}

void MSG::bitfield(uint8_t *msg, size_t *msgByte) {

}

void MSG::piece(uint8_t *msg, size_t *msgByte, int index, int begin, uint8_t *block, int blockLen) {

  bzero(msg, blockLen + 13);
  (uint32_t&)*msg = htonl(blockLen+9);
  msg[5] = 7;
  (uint32_t&)msg[6] = htonl(index);
  (uint32_t&)msg[10] = htonl(begin);

  for (int i = 0; i < blockLen; i++) {
    msg[14 + i] = block[begin];
    begin++;
  }
  *msgByte =  blockLen + 13;
}

void MSG::cancel(uint8_t *msg, size_t *msgByte, int index, int begin, int length) {
 bzero(msg,13);
  (uint32_t&)*msg = htonl(13);
  msg[4] = 8;
  (uint32_t&)msg[5] = htonl(index);
  (uint32_t&)msg[9] = htonl(begin);
  (uint32_t&)msg[13] = htonl(length);
  *msgByte = 17;
  
}


