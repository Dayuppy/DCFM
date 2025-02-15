#ifndef PATHTABLEENTRY_H
#define PATHTABLEENTRY_H

#include <cstdint>
#include <string>

struct PathTableEntry {
    uint8_t NameLength;
    uint8_t ExtendedAttributeRecordLength;
    uint32_t ExtentLocation;
    uint16_t ParentDirectoryNumber;
    std::string DirectoryIdentifier;
};

#endif // PATHTABLEENTRY_H