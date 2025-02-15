#include "ISO.h"
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <stack>
#include <unordered_set>
#include <regex>
#include <locale>
#include <codecvt>

ISO::ISO(const std::string& isoPath) : reader(isoPath, std::ios::binary), isoFileName(isoPath) {
    if (!reader.is_open()) {
        throw std::runtime_error("Failed to open ISO file.");
    }
    std::cout << "ISO file opened successfully: " << isoPath << std::endl;
}

ISO::~ISO() {
    Close();
}

void ISO::LoadISO() {
    ReadPrimaryVolumeDescriptor();
    ReadPathTable();
    BuildDirectoryRecords();
}

void ISO::ReadPrimaryVolumeDescriptor() {
    const int primaryVolumeDescriptorLBA = 16;
    const int logicalBlockSize = 2048;

    reader.seekg(primaryVolumeDescriptorLBA * logicalBlockSize, std::ios::beg);

    PrimaryVolumeDescriptor.Header.Type = reader.get();
    reader.read(PrimaryVolumeDescriptor.Header.Identifier, 5);
    PrimaryVolumeDescriptor.Header.Version = reader.get();

    reader.ignore(1);

    reader.read(PrimaryVolumeDescriptor.SystemIdentifier, 32);
    reader.read(PrimaryVolumeDescriptor.VolumeIdentifier, 32);

    reader.ignore(8);

    PrimaryVolumeDescriptor.VolumeSpaceSize.LittleEndian = Bytes::ReadUInt32(reader);
    PrimaryVolumeDescriptor.VolumeSpaceSize.BigEndian = Bytes::ReadUInt32BigEndian(reader);

    reader.ignore(32);

    PrimaryVolumeDescriptor.VolumeSetSize.LittleEndian = Bytes::ReadUInt16(reader);
    PrimaryVolumeDescriptor.VolumeSetSize.BigEndian = Bytes::ReadUInt16BigEndian(reader);

    PrimaryVolumeDescriptor.VolumeSequenceNumber.LittleEndian = Bytes::ReadUInt16(reader);
    PrimaryVolumeDescriptor.VolumeSequenceNumber.BigEndian = Bytes::ReadUInt16BigEndian(reader);

    PrimaryVolumeDescriptor.LogicalBlockSize.LittleEndian = Bytes::ReadUInt16(reader);
    PrimaryVolumeDescriptor.LogicalBlockSize.BigEndian = Bytes::ReadUInt16BigEndian(reader);

    PrimaryVolumeDescriptor.PathTableSize.LittleEndian = Bytes::ReadUInt32(reader);
    PrimaryVolumeDescriptor.PathTableSize.BigEndian = Bytes::ReadUInt32BigEndian(reader);

    PrimaryVolumeDescriptor.PathTableLocationLE = Bytes::ReadUInt32(reader);
    PrimaryVolumeDescriptor.OptionalPathTableLocationLE = Bytes::ReadUInt32(reader);
    PrimaryVolumeDescriptor.PathTableLocationBE = Bytes::ReadUInt32BigEndian(reader);
    PrimaryVolumeDescriptor.OptionalPathTableLocationBE = Bytes::ReadUInt32BigEndian(reader);

    DirectoryRecord rootDirRecord;
    if (!TryReadDirectoryRecord(rootDirRecord)) {
        throw std::runtime_error("Failed to read root directory record.");
    }
    PrimaryVolumeDescriptor.RootDirectoryRecord = new DirectoryRecord(rootDirRecord);

    std::cout << "Primary Volume Descriptor read successfully." << std::endl;
}

void ISO::ReadPathTable() {
    uint32_t pathTableLocation = PrimaryVolumeDescriptor.PathTableLocationLE;
    bool isBigEndian = false;

    if (pathTableLocation == 0) {
        pathTableLocation = PrimaryVolumeDescriptor.PathTableLocationBE;
        isBigEndian = true;
    }

    if (pathTableLocation == 0) {
        throw std::runtime_error("No valid Path Table Location found.");
    }

    uint32_t pathTableSize = isBigEndian ? PrimaryVolumeDescriptor.PathTableSize.BigEndian : PrimaryVolumeDescriptor.PathTableSize.LittleEndian;
    int64_t pathTableOffset = pathTableLocation * PrimaryVolumeDescriptor.LogicalBlockSize.LittleEndian;

    reader.seekg(pathTableOffset, std::ios::beg);

    PathTableEntries.clear();
    int64_t bytesRead = 0;

    while (bytesRead < pathTableSize) {
        PathTableEntry entry;
        entry.NameLength = reader.get();
        entry.ExtendedAttributeRecordLength = reader.get();
        bytesRead += 2;

        if (isBigEndian) {
            entry.ExtentLocation = Bytes::ReadUInt32BigEndian(reader);
            entry.ParentDirectoryNumber = Bytes::ReadUInt16BigEndian(reader);
        }
        else {
            entry.ExtentLocation = Bytes::ReadUInt32(reader);
            entry.ParentDirectoryNumber = Bytes::ReadUInt16(reader);
        }
        bytesRead += 6;

        if (entry.NameLength > 0) {
            std::vector<char> nameBuffer(entry.NameLength);
            reader.read(nameBuffer.data(), entry.NameLength);
            entry.DirectoryIdentifier.assign(nameBuffer.begin(), nameBuffer.end());
            bytesRead += entry.NameLength;

            if ((entry.NameLength & 1) == 1) {
                reader.ignore(1);
                bytesRead++;
            }
        }
        else {
            entry.DirectoryIdentifier.clear();
        }

        if (entry.ParentDirectoryNumber == 0) {
            throw std::runtime_error("Invalid ParentDirectoryNumber 0 in path table entry.");
        }

        PathTableEntries.push_back(entry);
    }

    std::cout << "Path Table read successfully." << std::endl;
}

void ISO::BuildDirectoryRecords() {
    if (PathTableEntries.empty()) {
        throw std::runtime_error("PathTableEntries are empty. Unable to build directory records.");
    }

    for (const auto& pathEntry : PathTableEntries) {
        std::string fullPath = GetFullPath(pathEntry);
        reader.seekg(pathEntry.ExtentLocation * PrimaryVolumeDescriptor.LogicalBlockSize.Value(), std::ios::beg);

        DirectoryRecord dirRecord;
        if (!TryReadDirectoryRecord(dirRecord)) {
            continue;
        }

        uint32_t dirDataLength = dirRecord.DataLength.Value();
        reader.seekg(pathEntry.ExtentLocation * PrimaryVolumeDescriptor.LogicalBlockSize.Value(), std::ios::beg);

        auto records = ReadDirectoryRecords(dirDataLength);

        for (const auto& record : records) {
            std::string recordName = record.FileIdentifier;
            size_t versionSeparatorIndex = recordName.find(';');
            if (versionSeparatorIndex != std::string::npos) {
                recordName = recordName.substr(0, versionSeparatorIndex);
            }

            recordName = recordName.erase(recordName.find_last_not_of('\0') + 1);
            recordName = recordName.erase(recordName.find_last_not_of('.') + 1);

            if (recordName == "." || recordName == ".." || recordName == "\0" || recordName == "\\" || recordName == ".\\") {
                continue;
            }

            std::string recordPath = std::filesystem::path(fullPath).append(recordName).string();
            std::replace(recordPath.begin(), recordPath.end(), '/', '\\');
            recordPath = recordPath.erase(recordPath.find_last_not_of('\\') + 1);

            DirectoryRecords[recordPath] = record;
            std::cout << "Record: " << recordPath << " at LBA " << record.ExtentLocation.Value() << ", IsFile: " << record.IsFile() << std::endl;
        }
    }

    std::cout << "Directory records built successfully." << std::endl;
}

std::string ISO::GetFullPath(const PathTableEntry& entry) {
    if (entry.ParentDirectoryNumber < 1 || entry.ParentDirectoryNumber > PathTableEntries.size()) {
        throw std::runtime_error("Invalid ParentDirectoryNumber in path table entry.");
    }

    int parentIndex = entry.ParentDirectoryNumber - 1;
    const auto& parentEntry = PathTableEntries[parentIndex];

    if (parentEntry.DirectoryIdentifier == entry.DirectoryIdentifier) {
        return entry.DirectoryIdentifier.empty() || entry.DirectoryIdentifier == "\0" || entry.DirectoryIdentifier == "\u0001" ? "." : entry.DirectoryIdentifier;
    }

    return std::filesystem::path(GetFullPath(parentEntry)).append(entry.DirectoryIdentifier).string();
}

DirectoryRecord ISO::ReadDirectoryRecord() {
    int64_t startPos = reader.tellg();

    DirectoryRecord dirRecord;
    dirRecord.Length = reader.get();
    if (dirRecord.Length == 0) {
        return dirRecord;
    }

    dirRecord.ExtendedAttributeRecordLength = reader.get();
    dirRecord.ExtentLocation.LittleEndian = Bytes::ReadUInt32(reader);
    dirRecord.ExtentLocation.BigEndian = Bytes::ReadUInt32BigEndian(reader);
    dirRecord.DataLength.LittleEndian = Bytes::ReadUInt32(reader);
    dirRecord.DataLength.BigEndian = Bytes::ReadUInt32BigEndian(reader);
    reader.read(reinterpret_cast<char*>(dirRecord.RecordingDateTime), 7);
    dirRecord.FileFlags = static_cast<FileFlags>(reader.get());
    dirRecord.FileUnitSize = reader.get();
    dirRecord.InterleaveGapSize = reader.get();
    dirRecord.VolumeSequenceNumber.LittleEndian = Bytes::ReadUInt16(reader);
    dirRecord.VolumeSequenceNumber.BigEndian = Bytes::ReadUInt16BigEndian(reader);
    dirRecord.FileIdentifierLength = reader.get();

    std::vector<char> idBuffer(dirRecord.FileIdentifierLength);
    reader.read(idBuffer.data(), dirRecord.FileIdentifierLength);
    std::copy(idBuffer.begin(), idBuffer.end(), dirRecord.FileIdentifier);

    if ((dirRecord.FileIdentifierLength & 1) == 0) {
        reader.ignore(1);
    }

    int64_t endPos = startPos + dirRecord.Length;
    reader.seekg(endPos, std::ios::beg);

    return dirRecord;
}

bool ISO::TryReadDirectoryRecord(DirectoryRecord& dirRecord) {
    int64_t startPos = reader.tellg();
    dirRecord.Length = reader.get();

    if (dirRecord.Length == 0) {
        dirRecord = {};
        return false;
    }

    dirRecord.ExtendedAttributeRecordLength = reader.get();
    dirRecord.ExtentLocation.LittleEndian = Bytes::ReadUInt32(reader);
    dirRecord.ExtentLocation.BigEndian = Bytes::ReadUInt32BigEndian(reader);
    dirRecord.DataLength.LittleEndian = Bytes::ReadUInt32(reader);
    dirRecord.DataLength.BigEndian = Bytes::ReadUInt32BigEndian(reader);
    reader.read(reinterpret_cast<char*>(dirRecord.RecordingDateTime), 7);
    dirRecord.FileFlags = static_cast<FileFlags>(reader.get());
    dirRecord.FileUnitSize = reader.get();
    dirRecord.InterleaveGapSize = reader.get();
    dirRecord.VolumeSequenceNumber.LittleEndian = Bytes::ReadUInt16(reader);
    dirRecord.VolumeSequenceNumber.BigEndian = Bytes::ReadUInt16BigEndian(reader);
    dirRecord.FileIdentifierLength = reader.get();

    std::vector<char> idBuffer(dirRecord.FileIdentifierLength);
    reader.read(idBuffer.data(), dirRecord.FileIdentifierLength);
    std::copy(idBuffer.begin(), idBuffer.end(), dirRecord.FileIdentifier);

    if ((dirRecord.FileIdentifierLength & 1) == 0) {
        reader.ignore(1);
    }

    int64_t endPos = startPos + dirRecord.Length;
    reader.seekg(endPos, std::ios::beg);

    return true;
}

std::vector<DirectoryRecord> ISO::ReadDirectoryRecords(uint32_t directorySize) {
    std::vector<DirectoryRecord> records;
    int64_t bytesRead = 0;

    while (bytesRead < directorySize) {
        int64_t recordStartPosition = reader.tellg();
        uint8_t length = reader.get();
        bytesRead += 1;

        if (length == 0) {
            int64_t positionInSector = bytesRead % PrimaryVolumeDescriptor.LogicalBlockSize.Value();
            if (positionInSector != 0) {
                int64_t padding = PrimaryVolumeDescriptor.LogicalBlockSize.Value() - positionInSector;
                if (padding != PrimaryVolumeDescriptor.LogicalBlockSize.Value()) {
                    reader.seekg(padding, std::ios::cur);
                    bytesRead += padding;
                }
            }
            break;
        }

        reader.seekg(-1, std::ios::cur);
        int64_t beforeReadPos = reader.tellg();
        DirectoryRecord record = ReadDirectoryRecord();
        int64_t afterReadPos = reader.tellg();

        bytesRead += afterReadPos - beforeReadPos;
        records.push_back(record);

        std::cout << "Read DirectoryRecord: Name='" << record.FileIdentifier << "', Length=" << static_cast<int>(record.Length) << ", IsFile=" << record.IsFile() << ", BytesRead=" << bytesRead << std::endl;
    }

    return records;
}

std::vector<uint8_t> ISO::ReadFileData(const DirectoryRecord& fileRecord) {
    reader.seekg(fileRecord.ExtentLocation.Value() * PrimaryVolumeDescriptor.LogicalBlockSize.Value(), std::ios::beg);
    std::vector<uint8_t> data(fileRecord.DataLength.Value());
    reader.read(reinterpret_cast<char*>(data.data()), fileRecord.DataLength.Value());
    return data;
}

void ISO::Close() {
    reader.close();
    std::cout << "ISO file closed." << std::endl;
}