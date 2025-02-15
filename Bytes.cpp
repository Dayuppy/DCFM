#include "Bytes.h"
#include <algorithm>
#include <fstream>

namespace Bytes {

    uint16_t ReverseUInt16(uint16_t value) {
        return (value >> 8) | (value << 8);
    }

    uint32_t ReverseUInt32(uint32_t value) {
        return (value >> 24) |
            ((value >> 8) & 0x0000FF00) |
            ((value << 8) & 0x00FF0000) |
            (value << 24);
    }

    uint32_t ReadUInt32(std::ifstream& stream) {
        uint32_t value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    }

    uint16_t ReadUInt16(std::ifstream& stream) {
        uint16_t value;
        stream.read(reinterpret_cast<char*>(&value), sizeof(value));
        return value;
    }

    uint32_t ReadUInt32BigEndian(std::ifstream& stream) {
        uint8_t bytes[4];
        stream.read(reinterpret_cast<char*>(bytes), 4);
        std::reverse(bytes, bytes + 4);
        return *reinterpret_cast<uint32_t*>(bytes);
    }

    uint16_t ReadUInt16BigEndian(std::ifstream& stream) {
        uint8_t bytes[2];
        stream.read(reinterpret_cast<char*>(bytes), 2);
        std::reverse(bytes, bytes + 2);
        return *reinterpret_cast<uint16_t*>(bytes);
    }

} // namespace Bytes