#include <boost/cstdint.hpp>

inline bool isBigEndian() {
    union {
        uint32_t i;
        char c[4];
    } Bint = {0x01020304};
    return Bint.c[0] == 1;
}

static int IsBigEndian = isBigEndian()

inline uint32_t swapEndianness(uint32_t in) {
    unsigned char b1, b2, b3, b4;

    b1 = in & 255;
    b2 = ( in >> 8 ) & 255;
    b3 = ( in >> 16 ) & 255;
    b4 = ( in >> 24 ) & 255;

    return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

inline uint32_t toLittleEndian(uint32_t in) {
    return isBigEndian() ? in : swapEndianness(in);
}

inline uint32_t toBigEndian(uint32_t in) {
    return isBigEndian() ? swapEndianness(in) : in;
}
