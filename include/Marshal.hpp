#ifndef MARSHALL_HPP
#define MARSHALL_HPP

#define TYPES_LIST(code) \
    code(INT8, 0x00)\
    code(INT16, 0x01) \
    code(INT32, 0x02) \
    code(INT64, 0x03) \
    code(OFF8, 0x04) \
    code(OFF16, 0x05) \
    code(OFF32, 0x06) \
    code(BLOCK32, 0x08) \
    code(BLOCK64, 0x13) \
    code(SHORTSTR, 0x09) \
    code(LONGSTR, 0x0A) \
    code(FLOAT1, 0x0B) \
    code(FLOAT2, 0x0C) \
    code(SHORTFLOATARRAY1, 0x0D) \
    code(SHORTFLOATARRAY2, 0x0E) \
    code(LONGFLOATARRAY1, 0x07) \
    code(LONGFLOATARRAY2, 0x0F) \
    code(CHECKSUM, 0x10) \
    code(CLOSURE, 0x11) \
    code(CUSTOMDATA, 0x12) \
    code(SMALLELEMENT, 0X20) 

enum MarshallTypes {
    #define DEFINE_ENUM(type, code) \
        type = code,
    TYPES_LIST(DEFINE_ENUM)
    #undef DEFINE_ENUM
};

struct MarshallType {
    const char Name[32];
};

void printType(int type) {
    static MarshallType StringTypes[32] = {{0}};
    if (StringTypes[0].Name[0] = 0) {
        #define DEFINE_INIT_STRINGTYPES(type, code) \
            StringTypes[code] = #type;
        TYPES_LIST(DEFINE_INIT_STRINGTYPES)
        #undef DEFINE_INIT_STRINGTYPES
    }
    std::cout << StringTypes[type] << std::endl;
}

#endif //MARSHALL_HPP
