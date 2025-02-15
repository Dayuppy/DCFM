#ifndef PRIMARYVOLUMEDESCRIPTOR_H
#define PRIMARYVOLUMEDESCRIPTOR_H

#include "VolumeDescriptorHeader.h"
#include "BothEndianUInt32.h"
#include "BothEndianUInt16.h"
#include "DirectoryRecord.h"

struct PrimaryVolumeDescriptor {
    VolumeDescriptorHeader Header;
    char SystemIdentifier[32]; // 32 bytes
    char VolumeIdentifier[32]; // 32 bytes
    uint8_t UnusedField1[8]; // 8 bytes
    BothEndianUInt32 VolumeSpaceSize; // 8 bytes
    uint8_t EscapeSequences[32]; // 32 bytes (Unused)
    BothEndianUInt16 VolumeSetSize; // 4 bytes
    BothEndianUInt16 VolumeSequenceNumber; // 4 bytes
    BothEndianUInt16 LogicalBlockSize; // 4 bytes
    BothEndianUInt32 PathTableSize; // 8 bytes
    uint32_t PathTableLocationLE; // 4 bytes (Type L Path Table)
    uint32_t OptionalPathTableLocationLE; // 4 bytes
    uint32_t PathTableLocationBE; // 4 bytes (Type M Path Table)
    uint32_t OptionalPathTableLocationBE; // 4 bytes
    DirectoryRecord RootDirectoryRecord; // 34 bytes
};
#endif // PRIMARYVOLUMEDESCRIPTOR_H