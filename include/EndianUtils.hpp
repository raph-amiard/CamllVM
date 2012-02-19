#ifndef ENDIANUTILS_HPP
#define ENDIANUTILS_HPP

#include <boost/cstdint.hpp>
#include <fstream>
#include <iostream>

inline bool isBigEndian() {
    union {
        uint32_t i;
        char c[4];
    } Bint = {0x01020304};
    return Bint.c[0] == 1;
}

static int IsBigEndian = isBigEndian();

template <typename T> inline T swapEndianness(T in) {
    unsigned char b1, b2, b3, b4;

    b1 = in & 255;
    b2 = ( in >> 8 ) & 255;
    b3 = ( in >> 16 ) & 255;
    b4 = ( in >> 24 ) & 255;

    return ((T)b1 << 24) + ((T)b2 << 16) + ((T)b3 << 8) + b4;
}

template <typename T> inline T toLittleEndian(T in) {
    return isBigEndian() ? in : swapEndianness(in);
}

template <typename T> inline T toBigEndian(T in) {
    return isBigEndian() ? swapEndianness(in) : in;
}

#endif
