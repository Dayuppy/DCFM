#include "ISO.h"
#include "DirectoryRecord.h"
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <unordered_set>
#include <algorithm>

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
    reader.read(reinterpret_cast<char*>(&PrimaryVolumeDescriptor), sizeof(PrimaryVolumeDescriptor));

    if (strncmp(PrimaryVolumeDescriptor.Header.Identifier, "CD001", 5) != 0) {
        throw std::runtime_error("Invalid Primary Volume Descriptor.");
    }

    DirectoryRecord rootDirRecord;
    if (!TryReadDirectoryRecord(rootDirRecord)) {
        throw std::runtime_error("Failed to read root directory record.");
    }
    PrimaryVolumeDescriptor.RootDirectoryRecord = rootDirRecord;

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
        } else {
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
        } else {
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

    std::unordered_set<std::string> seenEntries;
    std::vector<std::pair<std::string, DirectoryRecord>> allRecords;

    for (const auto& pathEntry : PathTableEntries) {
        std::string fullPath = GetFullPath(pathEntry);
        reader.seekg(pathEntry.ExtentLocation * PrimaryVolumeDescriptor.LogicalBlockSize.Value(), std::ios::beg);

        uint32_t dirDataLength = PrimaryVolumeDescriptor.LogicalBlockSize.Value();
        auto records = ReadDirectoryRecords(dirDataLength);

        for (auto& record : records) {
            std::string recordName(record.FileIdentifier, record.FileIdentifierLength);

            // Ensure the record name is valid
            if (recordName.empty() || recordName == "." || recordName == "..") {
                continue;
            }

            // Handle ISO-specific versioning like ";1"
            size_t versionSeparatorIndex = recordName.find(';');
            if (versionSeparatorIndex != std::string::npos) {
                recordName = recordName.substr(0, versionSeparatorIndex);
            }

            // Trim null characters and handle periods appropriately
            recordName.erase(std::remove(recordName.begin(), recordName.end(), '\0'), recordName.end());
            
            std::string recordPath = std::filesystem::path(fullPath).append(recordName).string();
            std::replace(recordPath.begin(), recordPath.end(), '/', '\\');
            recordPath.erase(std::find_if(recordPath.rbegin(), recordPath.rend(), [](unsigned char ch) {
                return ch != '\\';
            }).base(), recordPath.end());

            // Add to the list of all records
            allRecords.emplace_back(recordPath, record);
        }
    }

    // Separate records into directories and files
    for (const auto& recordPair : allRecords) {
        const auto& recordPath = recordPair.first;
        const auto& record = recordPair.second;

        if (!seenEntries.insert(recordPath).second) {
            continue; // Skip duplicates
        }

        if (record.IsDirectory()) {
            DirectoryRecords[recordPath] = record;
        } else {
            FileRecords[recordPath] = record;
        }
        std::cout << "Record: " << recordPath << " at LBA " << record.ExtentLocation.Value() << ", IsDirectory: " << record.IsDirectory() << ", IsFile: " << !record.IsDirectory() << std::endl;
    }

    std::cout << "Directory records built successfully." << std::endl;
    std::cout << "File Records count: " << FileRecords.size() << std::endl;
    for (const auto& fileRecord : FileRecords) {
        std::cout << "File: " << fileRecord.first << " Size: " << fileRecord.second.DataLength.Value() << " bytes" << std::endl;
    }
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

    reader.read(dirRecord.FileIdentifier, dirRecord.FileIdentifierLength);
    dirRecord.FileIdentifier[dirRecord.FileIdentifierLength] = '\0'; // Null-terminate

    if ((dirRecord.FileIdentifierLength & 1) == 0) {
        reader.ignore(1);
    }

    return dirRecord;
}

bool ISO::TryReadDirectoryRecord(DirectoryRecord& dirRecord) {
    dirRecord.Length = reader.get();
    if (dirRecord.Length == 0) {
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

    reader.read(dirRecord.FileIdentifier, dirRecord.FileIdentifierLength);
    dirRecord.FileIdentifier[dirRecord.FileIdentifierLength] = '\0'; // Null-terminate

    if ((dirRecord.FileIdentifierLength & 1) == 0) {
        reader.ignore(1);
    }

    return true;
}

std::vector<DirectoryRecord> ISO::ReadDirectoryRecords(uint32_t directorySize) {
    std::vector<DirectoryRecord> records;
    int64_t bytesRead = 0;

    while (bytesRead < directorySize) {
        DirectoryRecord record;
        if (!TryReadDirectoryRecord(record)) {
            break;
        }

        records.push_back(record);
        bytesRead += record.Length;
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