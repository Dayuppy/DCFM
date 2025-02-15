#ifndef VOLUMEDESCRIPTORHEADER_H
#define VOLUMEDESCRIPTORHEADER_H

#include <cstdint>

struct VolumeDescriptorHeader {
    uint8_t Type; // Volume Descriptor Type
    char Identifier[5]; // Standard Identifier (should be "CD001")
    uint8_t Version; // Volume Descriptor Version
};


#endif // VOLUMEDESCRIPTORHEADER_H