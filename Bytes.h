#ifndef BYTES_H
#define BYTES_H

#include <cstdint>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>

namespace Bytes {

    uint16_t ReverseUInt16(uint16_t value);
    uint32_t ReverseUInt32(uint32_t value);
    uint32_t ReadUInt32(std::ifstream& stream);
    uint16_t ReadUInt16(std::ifstream& stream);
    uint32_t ReadUInt32BigEndian(std::ifstream& stream);
    uint16_t ReadUInt16BigEndian(std::ifstream& stream);

} // namespace Bytes

#endif // BYTES_H