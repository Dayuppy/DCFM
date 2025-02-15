#ifndef HED_H
#define HED_H

#include <cstdint>
#include <string>

struct HED {
    char Name[64];
    uint32_t Offset;
    uint32_t Size;
    uint32_t ID;
    uint32_t SomeID;
};

#endif // HED_H