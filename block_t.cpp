#include <string>
#include <vector>
#include <array>

#include "block_t.h"

block_t::block_t(int index, int b, uint8_t bl[]) {

    piece_index = index;
    offset = b;
    copy(begin(bl), end(bl), begin(data));
}

int block_t::get_index() {
    return piece_index;
}

int block_t::get_offset() {
    return offset;
}

uint8_t block_t::get_data() {
    return data;
}

