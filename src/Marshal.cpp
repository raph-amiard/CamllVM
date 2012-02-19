#include <Marshal.hpp>


void printType(MarshalTypeId type) {

    static MarshalType StringTypes[32];
    static bool InitStringTypes = true;

    if (InitStringTypes) {
        #define DEFINE_INIT_STRINGTYPES(t, c) \
            strcpy(StringTypes[c].Name, #t);
        TYPES_LIST(DEFINE_INIT_STRINGTYPES)
        #undef DEFINE_INIT_STRINGTYPES
        InitStringTypes = false;
    }

    if (type < 0X12) 
        std::cout << StringTypes[type].Name << std::endl;
    else
        std::cout << "Invalid type" << std::endl;
}

