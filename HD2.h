#ifndef HD2_H
#define HD2_H

#include <cstdint>

struct HD2 {
    uint32_t NameOffset;
    uint32_t Zero1;
    uint32_t Zero2;
    uint32_t Zero3;
    uint32_t Offset;
    uint32_t Size;
    uint32_t LBAOffset;
    uint32_t LBAExtent;
};

#endif // HD2_H