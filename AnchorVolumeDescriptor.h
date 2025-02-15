
#ifndef ANCHORVOLUMEDESCRIPTORHEADER_H
#define ANCHORVOLUMEDESCRIPTORHEADER_H

#include <cstdint>

struct AnchorVolumeDescriptorPointer {
    uint32_t MainVolumeDescriptorSequenceExtent; // Location of the main volume descriptor sequence
    uint32_t ReserveVolumeDescriptorSequenceExtent; // Location of the reserve volume descriptor sequence
    uint8_t Reserved[480]; // Padding (480 bytes)
};

#endif // ANCHORVOLUMEDESCRIPTORHEADER_H