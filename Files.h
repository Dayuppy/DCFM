#ifndef FILES_H
#define FILES_H

#include <string>
#include <vector>
#include <fstream>
#include "ISO9660.h"
#include "HED.h"
#include "HD2.h"

namespace Files {

    std::string IdentifyFileType(const std::vector<uint8_t>& headerBytes);
    int ReadFile(const std::vector<uint8_t>& bytes, int64_t offset, int64_t length);
    int ReadHED(const std::string& filepath);
    int ReadHD2(const std::string& filepath);
    std::vector<HED> ReadHEDEntries(std::ifstream& stream);
    std::vector<HD2> ReadHD2Entries(std::ifstream& stream);

    class FileItem {
    public:
        std::string FileName;
        std::string FilePath;
        DirectoryRecord Record;
    };

} // namespace Files

#endif // FILES_H