#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class SectionType {
    nospamkeys = 1,
    dht = 2,
    friends = 3,
    name = 4,
    statusmessage = 5,
    status = 6,
    tcpRelay = 10,
    pathNode = 11,
    conferences = 20,
    eof = 255,
};

SectionType readSectionType(uint8_t*& data);

struct SectionHeader {
    uint32_t size;
    uint8_t* data;
    SectionType type; // may hold unknown enum values
};

SectionHeader getSection(uint8_t*& data);
std::string sectionToString(SectionType section);
std::vector<SectionHeader> getAllSections(uint8_t* data);
