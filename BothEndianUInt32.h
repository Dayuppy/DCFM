#ifndef BOTHENDIANUINT32_H
#define BOTHENDIANUINT32_H

#include <cstdint>
#include <intrin.h> // Use for _byteswap_ulong

struct BothEndianUInt32 {
    uint32_t LittleEndian;
    uint32_t BigEndian;

    uint32_t Value() const {
        return LittleEndian;
    }

    void SetValue(uint32_t value) {
        LittleEndian = value;
        BigEndian = _byteswap_ulong(value); // Use built-in function for byte swap
    }
};

#endif // BOTHENDIANUINT32_H