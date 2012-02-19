#ifndef MARSHALL_HPP
#define MARSHALL_HPP

#include <cstring>
#include <iostream>

#define TYPES_LIST(code) \
    code(M_INT8, 0x00)\
    code(M_INT16, 0x01) \
    code(M_INT32, 0x02) \
    code(M_INT64, 0x03) \
    code(M_OFF8, 0x04) \
    code(M_OFF16, 0x05) \
    code(M_OFF32, 0x06) \
    code(M_BLOCK32, 0x08) \
    code(M_BLOCK64, 0x13) \
    code(M_SHORTSTR, 0x09) \
    code(M_LONGSTR, 0x0A) \
    code(M_FLOAT1, 0x0B) \
    code(M_FLOAT2, 0x0C) \
    code(M_SHORTFLOATARRAY1, 0x0D) \
    code(M_SHORTFLOATARRAY2, 0x0E) \
    code(M_LONGFLOATARRAY1, 0x07) \
    code(M_LONGFLOATARRAY2, 0x0F) \
    code(M_CHECKSUM, 0x10) \
    code(M_CLOSURE, 0x11) \
    code(M_CUSTOMDATA, 0x12) \
    code(M_SMALLELEMENT, 0X20) 

enum MarshalTypeId {
    #define DEFINE_ENUM(type, code) \
        type = code,
    TYPES_LIST(DEFINE_ENUM)
    #undef DEFINE_ENUM
};

struct MarshalType {
    char Name[32];
};

void printType(MarshalTypeId type); 

#endif //MARSHALL_HPP
