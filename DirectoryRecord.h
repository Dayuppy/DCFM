#ifndef DIRECTORYRECORD_H
#define DIRECTORYRECORD_H

#include "BothEndianUInt32.h"
#include "BothEndianUInt16.h"
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

enum class FileFlags : uint8_t {
    None = 0,
    Hidden = 1 << 0,
    Directory = 1 << 1,
    AssociatedFile = 1 << 2,
    Record = 1 << 3,
    Protection = 1 << 4,
    MultiExtent = 1 << 7
};

constexpr bool HasFlags(FileFlags value, FileFlags flag) {
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

struct DirectoryRecord {
    uint8_t Length;
    uint8_t ExtendedAttributeRecordLength;
    BothEndianUInt32 ExtentLocation;
    BothEndianUInt32 DataLength;
    uint8_t RecordingDateTime[7];
    FileFlags FileFlags;
    uint8_t FileUnitSize;
    uint8_t InterleaveGapSize;
    BothEndianUInt16 VolumeSequenceNumber;
    uint8_t FileIdentifierLength;
    char FileIdentifier[256]; // Ensure buffer is large enough for file names

    bool IsDirectory() const {
        return HasFlags(FileFlags, FileFlags::Directory);
    }

    uint32_t GetSize() const {
        return DataLength.Value();
    }

    std::string GetFormattedDateTime() const {
        std::tm tm = {};
        tm.tm_year = RecordingDateTime[0] + 70; // Years since 1900
        tm.tm_mon = RecordingDateTime[1] - 1;   // Months since January
        tm.tm_mday = RecordingDateTime[2];
        tm.tm_hour = RecordingDateTime[3];
        tm.tm_min = RecordingDateTime[4];
        tm.tm_sec = RecordingDateTime[5];
        tm.tm_isdst = -1;

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};

#endif // DIRECTORYRECORD_H