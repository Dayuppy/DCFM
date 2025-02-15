#ifndef ISO_H
#define ISO_H

#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <memory>
#include "ISO9660.h"
#include "Bytes.h"

class ISO {
public:
    ISO(const std::string& isoPath);
    ~ISO();

    void LoadISO();
    std::vector<uint8_t> ReadFileData(const DirectoryRecord& fileRecord);
    std::string GetFileName() const { return isoFileName; }
    const std::unordered_map<std::string, DirectoryRecord>& GetDirectoryRecords() const { return DirectoryRecords; }

private:
    void ReadPrimaryVolumeDescriptor();
    void ReadPathTable();
    void BuildDirectoryRecords();
    std::string GetFullPath(const PathTableEntry& entry);
    DirectoryRecord ReadDirectoryRecord();
    bool TryReadDirectoryRecord(DirectoryRecord& dirRecord);
    std::vector<DirectoryRecord> ReadDirectoryRecords(uint32_t directorySize);

    void Close();

    std::ifstream reader;
    PrimaryVolumeDescriptor PrimaryVolumeDescriptor;
    std::vector<PathTableEntry> PathTableEntries;
    std::unordered_map<std::string, DirectoryRecord> DirectoryRecords;
    std::string isoFileName;
}; 

#endif //ISO_H
