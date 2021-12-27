#ifndef PIECES_H
#define PIECES_H

#include <string>
#include <array>
#include <vector>
#define MAX_BLOCK 16384

using namespace std;

class block_t {

public:
    int piece_index;
    int offset;
    uint8_t data[MAX_BLOCK];

    block_t(int index, int b, unsigned char[] bl); // Takes the piece index, the offset of the piece, and the block
    int get_index();
    int get_offset();
    uint8_t get_data();
};


#endif