#ifndef PRIMARYVOLUMEDESCRIPTOR_H
#define PRIMARYVOLUMEDESCRIPTOR_H

#include "VolumeDescriptorHeader.h"
#include "BothEndianUInt32.h"
#include "BothEndianUInt16.h"
#include "DirectoryRecord.h"

struct PrimaryVolumeDescriptor {
    VolumeDescriptorHeader Header;
    char SystemIdentifier[32];
    char VolumeIdentifier[32];
    uint8_t UnusedField1[8];
    BothEndianUInt32 VolumeSpaceSize;
    uint8_t EscapeSequences[32];
    BothEndianUInt16 VolumeSetSize;
    BothEndianUInt16 VolumeSequenceNumber;
    BothEndianUInt16 LogicalBlockSize;
    BothEndianUInt32 PathTableSize;
    uint32_t PathTableLocationLE;
    uint32_t OptionalPathTableLocationLE;
    uint32_t PathTableLocationBE;
    uint32_t OptionalPathTableLocationBE;
    DirectoryRecord* RootDirectoryRecord;
};

#endif // PRIMARYVOLUMEDESCRIPTOR_H