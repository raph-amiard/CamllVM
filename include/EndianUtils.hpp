#include <boost/cstdint.hpp>

inline bool IsBigEndian() {
    union {
        uint32_t i;
        char c[4];
    } Bint = {0x01020304};
    return Bint.c[0] == 1;
}

inline uint32_t SwapEndianness(uint32_t in) {
    unsigned char b1, b2, b3, b4;

    b1 = in & 255;
    b2 = ( in >> 8 ) & 255;
    b3 = ( in >> 16 ) & 255;
    b4 = ( in >> 24 ) & 255;

    return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

inline uint32_t ToLittleEndian(uint32_t in) {
    return IsBigEndian() ? in : SwapEndianness(in);
}

