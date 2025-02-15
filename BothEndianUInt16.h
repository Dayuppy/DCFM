#ifndef BOTHENDIANUINT16_H
#define BOTHENDIANUINT16_H

#include <cstdint>
#include <intrin.h> // Use for _byteswap_ushort

struct BothEndianUInt16 {
    uint16_t LittleEndian;
    uint16_t BigEndian;

    uint16_t Value() const {
        return LittleEndian;
    }

    void SetValue(uint16_t value) {
        LittleEndian = value;
        BigEndian = _byteswap_ushort(value); // Use built-in function for byte swap
    }
};

#endif // BOTHENDIANUINT16_H