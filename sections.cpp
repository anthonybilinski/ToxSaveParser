#include "sections.h"
#include <cstdint>    // for uint16_t, uint32_t, uint8_t
#include <memory>     // for allocator_traits<>::value_type
#include <stdexcept>  // for runtime_error
#include <vector>     // for vector
#include "utils.h"    // for dataToNumber

SectionHeader getSection(uint8_t*& data)
{
    SectionHeader header;
    header.size = dataToNumber<uint32_t>(data);
    header.type = readSectionType(data);

    const static uint16_t sectionMagic = 0x01ce;
    if (dataToNumber<uint16_t>(data) != sectionMagic) {
        throw std::runtime_error("Couldn't parse section magic bytes.");
    }
    header.data = data;

    return header;
}

SectionType readSectionType(uint8_t*& data)
{
    // don't check if it's valid, since we still want to handle unknown sections
    return static_cast<SectionType>(dataToNumber<uint16_t>(data));
}

std::string sectionToString(SectionType section)
{
    switch(section) {
         case SectionType::nospamkeys:
            return "Nospam and Keys";
        case SectionType::dht:
            return "DHT Nodes"; // combining DHT with its one nodes inner section for nicer presentation
        case SectionType::friends:
            return "Friends";
        case SectionType::name:
            return "Name";
        case SectionType::statusmessage:
            return "Status Message";
        case SectionType::status:
            return "Status";
        case SectionType::tcpRelay:
            return "Tcp Relays";
        case SectionType::pathNode:
            return "Path Nodes";
        case SectionType::conferences:
            return "Conferences";
        case SectionType::eof:
            return "EOF";
    }
    return "Unknown Section";
}

std::vector<SectionHeader> getAllSections(uint8_t* data)
{
    std::vector<SectionHeader> sections;

    while (true) {
        auto section = getSection(data);
        if (section.type == SectionType::eof) {
            return sections;
        }
        sections.emplace_back(section);
        data += sections.back().size;
    }
}
