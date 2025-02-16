#include "ISO.h"
#include "DirectoryRecord.h"
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <unordered_set>
#include <algorithm>
#include <Windows.h>

ISO::ISO(const std::string& isoPath)
    : reader(isoPath, std::ios::binary), isoFileName(isoPath) {
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

    uint32_t pathTableSize = isBigEndian
        ? PrimaryVolumeDescriptor.PathTableSize.BigEndian
        : PrimaryVolumeDescriptor.PathTableSize.LittleEndian;
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

std::string ISO::GetRootFolderName() const {
    return std::filesystem::path(isoFileName).stem().string();
}

std::string ISO::GetFullPath(const PathTableEntry& entry) {
    // Base case: if this entry's parent is the root (ISO spec defines the root's ParentDirectoryNumber as 1),
    // then return the ISO's root folder name plus the entry's identifier (if nonempty).
    if (entry.ParentDirectoryNumber == 1) {
        std::string root = GetRootFolderName();
        if (!entry.DirectoryIdentifier.empty() &&
            entry.DirectoryIdentifier != "\0" &&
            entry.DirectoryIdentifier != "\u0001")
        {
            return root + "\\" + entry.DirectoryIdentifier;
        }
        return root;
    }

    int parentIndex = entry.ParentDirectoryNumber - 1;
    if (parentIndex < 0 || parentIndex >= PathTableEntries.size()) {
        throw std::runtime_error("Invalid ParentDirectoryNumber in path table entry.");
    }

    const auto& parentEntry = PathTableEntries[parentIndex];
    std::filesystem::path fullPath = std::filesystem::path(GetFullPath(parentEntry)) / entry.DirectoryIdentifier;
    return fullPath.string();
}

void ISO::BuildDirectoryRecords() {
    if (PathTableEntries.empty()) {
        throw std::runtime_error("PathTableEntries are empty. Unable to build directory records.");
    }

    std::unordered_set<uint32_t> seenLBAs;
    for (const auto& pathEntry : PathTableEntries) {
        std::string fullPath = GetFullPath(pathEntry);

        reader.seekg(pathEntry.ExtentLocation * PrimaryVolumeDescriptor.LogicalBlockSize.Value(), std::ios::beg);
        uint32_t dirDataLength = PrimaryVolumeDescriptor.LogicalBlockSize.Value();
        auto records = ReadDirectoryRecords(dirDataLength);

        for (const auto& record : records) {
            std::string recordName(record.FileIdentifier, record.FileIdentifierLength);
            std::cout << "Raw Record Name: [" << recordName << "]" << std::endl;

            // Build the record path relative to the directory.
            std::string recordPath = (std::filesystem::path(fullPath) / recordName).string();
            if (!recordPath.empty() && recordPath.back() == '\\') {
                recordPath.pop_back();
            }

            if (!seenLBAs.insert(record.ExtentLocation.Value()).second) {
                continue;
            }

            if (record.IsDirectory()) {
                DirectoryRecords[recordPath] = record;
                //std::cout << "Folder Record: " << recordPath << " at LBA " << record.ExtentLocation.Value() << std::endl;
            }
            else {
                FileRecords[recordPath] = record;
                //std::cout << "File Record: " << recordPath << " at LBA " << record.ExtentLocation.Value() << std::endl;
            }
        }
    }

    std::cout << "Directory records built successfully." << std::endl;
    std::cout << "File Records count: " << FileRecords.size() << std::endl;
    for (const auto& fileRecord : FileRecords) {
        std::cout << "File: " << fileRecord.first << " Size: " << fileRecord.second.DataLength.Value() << " bytes" << std::endl;
    }
}

bool ISO::TryReadDirectoryRecord(DirectoryRecord& dirRecord) {
    int64_t startPos = reader.tellg();
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
    dirRecord.FileIdentifier[dirRecord.FileIdentifierLength] = '\0'; // null-terminate

    // According to ISO 9660, if the File Identifier Length is even, a pad byte is present.
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