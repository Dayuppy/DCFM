#include "Files.h"
#include "Bytes.h"
#include <iostream>
#include <fstream>

namespace Files {

    std::string IdentifyFileType(const std::vector<uint8_t>& headerBytes) {
        if (!headerBytes.empty() && headerBytes[0] == 0x01) {
            return "ISO File";
        }
        return "Unknown Type";
    }

    int ReadFile(const std::vector<uint8_t>& bytes, int64_t offset, int64_t length) {
        std::cout << "Reading file from offset " << offset << ", length " << length << std::endl;
        return 0;
    }

    int ReadHED(const std::string& filepath) {
        std::ifstream stream(filepath, std::ios::binary);
        if (!stream) {
            std::cerr << "Failed to open HED file: " << filepath << std::endl;
            return -1;
        }

        auto entries = ReadHEDEntries(stream);
        // Process entries
        return 0;
    }

    int ReadHD2(const std::string& filepath) {
        std::ifstream stream(filepath, std::ios::binary);
        if (!stream) {
            std::cerr << "Failed to open HD2 file: " << filepath << std::endl;
            return -1;
        }

        auto entries = ReadHD2Entries(stream);
        // Process entries
        return 0;
    }

    std::vector<HED> ReadHEDEntries(std::ifstream& stream) {
        std::vector<HED> hedEntries;

        while (stream.tellg() < stream.end) {
            HED entry;
            stream.read(entry.Name, 64);
            entry.Offset = Bytes::ReadUInt32(stream);
            entry.Size = Bytes::ReadUInt32(stream);
            entry.ID = Bytes::ReadUInt32(stream);
            entry.SomeID = Bytes::ReadUInt32(stream);
            hedEntries.push_back(entry);
        }

        return hedEntries;
    }

    std::vector<HD2> ReadHD2Entries(std::ifstream& stream) {
        std::vector<HD2> hd2Entries;

        while (stream.tellg() < stream.end) {
            HD2 entry;
            entry.NameOffset = Bytes::ReadUInt32(stream);
            entry.Zero1 = Bytes::ReadUInt32(stream);
            entry.Zero2 = Bytes::ReadUInt32(stream);
            entry.Zero3 = Bytes::ReadUInt32(stream);
            entry.Offset = Bytes::ReadUInt32(stream);
            entry.Size = Bytes::ReadUInt32(stream);
            entry.LBAOffset = Bytes::ReadUInt32(stream);
            entry.LBAExtent = Bytes::ReadUInt32(stream);
            hd2Entries.push_back(entry);
        }

        return hd2Entries;
    }

} // namespace Files