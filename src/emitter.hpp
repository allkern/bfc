#pragma once

#include <cstdint>

namespace bfc {
    namespace backend {
        uint64_t bswapn(uint64_t a, size_t s) {
            return (((a & 0x00000000000000ffull) << 56) | 
                    ((a & 0x000000000000ff00ull) << 40) | 
                    ((a & 0x0000000000ff0000ull) << 24) | 
                    ((a & 0x00000000ff000000ull) <<  8) | 
                    ((a & 0x000000ff00000000ull) >>  8) | 
                    ((a & 0x0000ff0000000000ull) >> 24) | 
                    ((a & 0x00ff000000000000ull) >> 40) | 
                    ((a & 0xff00000000000000ull) >> 56)) >> (64 - (s << 3));
        }

        namespace emitter {
            struct return_t {
                size_t size;
                uint64_t data;
            };

    #ifdef BFC_TARGET_X64
            return_t add(uint8_t i) { return { 6, 0x664183045c00ul | i }; }
            return_t sub(uint8_t i) { return { 6, 0x6641832c5c00ul | i }; }
            return_t adp(uint8_t i) { return { 4, 0x4883c300ul | i }; }
            return_t sdp(uint8_t i) { return { 4, 0x4883eb00ul | i }; }
            return_t beq(int32_t i) { return { 6, 0x0f8400000000ul | bswapn(i, 4) }; }
            return_t bne(int32_t i) { return { 6, 0x0f8500000000ul | bswapn(i, 4) }; }
            return_t call(int32_t r) { return { 6, 0xff1500000000ul | bswapn(r, 4) }; }
            return_t mov(uint16_t i) { return { 7, 0x6641c7045c0000ull | bswapn(i, 2) }; }
    #endif
        }
    }
}