#ifndef VOLUMEDESCRIPTORHEADER_H
#define VOLUMEDESCRIPTORHEADER_H

#include <cstdint>

struct VolumeDescriptorHeader {
    uint8_t Type;
    char Identifier[5];
    uint8_t Version;
};

#endif // VOLUMEDESCRIPTORHEADER_H