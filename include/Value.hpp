#include <cstdint>
#include <Block.hpp>

class Value {
    union {
        uint32_t Int32Val;
        uint64_t Int64Val;
        Block* BlockVal;
    } Val;

    inline uint32_t asInt() { return Val.Int32Val; }
    inline uint64_t asInt64() { return Val.Int64Val; }
    inline Block* asBlock() { return Val.BlockVal; }

};
