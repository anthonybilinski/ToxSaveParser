#include <bits/exception.h>      // for exception
#include <fcntl.h>               // for open, O_RDONLY
#include <qbytearray.h>          // for QByteArray
#include <qcommandlineparser.h>  // for QCommandLineParser
#include <qcoreapplication.h>    // for QCoreApplication
#include <qdatetime.h>           // for QDateTime
#include <qglobal.h>             // for qSwap
#include <qjsonarray.h>          // for QJsonArray
#include <qjsondocument.h>       // for QJsonDocument, QJsonDocument::Indented
#include <qjsonobject.h>         // for QJsonObject
#include <qjsonvalue.h>          // for QJsonValue
#include <qnamespace.h>          // for ISODate
#include <qstring.h>             // for QString
#include <qstringlist.h>         // for QStringList
#include <qtconcurrentmap.h>     // for blockingMappedReduced
#include <stdio.h>               // for perror, fprintf, sprintf, stderr
#include <stdlib.h>              // for exit, EXIT_FAILURE
#include <sys/mman.h>            // for mmap, munmap, MAP_FAILED, MAP_SHARED
#include <sys/stat.h>            // for stat, fstat, mode_t
#include <unistd.h>              // for close
#include <cstdint>               // for uint8_t, uint16_t, uint32_t, uint64_t
#include <iostream>              // for endl, operator<<, basic_ostream, ost...
#include <iterator>              // for advance
#include <stdexcept>             // for runtime_error, invalid_argument
#include <string>                // for string, operator<<
#include <vector>                // for vector
#include "nodeinfo.h"            // for getNodeInfos
#include "sections.h"            // for SectionHeader, SectionType, sectionT...
#include "utils.h"               // for dataToNumber, readString, readHexData

enum class DhtSection {
    nodes = 4
};

DhtSection getDhtSectionType(uint8_t*& data)
{
    static const uint16_t dhtInnerSectionHeader = 0x11ce;
    auto sectionVal = dataToNumber<uint16_t>(data);
    auto header = dataToNumber<uint16_t>(data);

    if (header != dhtInnerSectionHeader) {
        throw std::runtime_error("Couldn't parse DHT state cookie.");
    }
    return static_cast<DhtSection>(sectionVal);
}

void addDataToArray(uint8_t*& data, int size, QJsonArray& node)
{
    std::vector<char> hexString(size*2, 0);

    for (int i = 0; i < size; ++i)
    {
        sprintf(hexString.data()+i*2, "%02X", data[0]);
        data++;
    }

    std::string str(hexString.begin(), hexString.end());
    node.append(str.c_str());
}

QJsonObject getNoSpamKeys(uint8_t*& data)
{
    QJsonObject node;
    node.insert("Nospam", readHexData(data, 4).c_str());
    node.insert("Long term public key", readHexData(data, 32).c_str());
    node.insert("Long term secret key", readHexData(data, 32).c_str());
    return node;
}

QJsonArray getDhtSection(uint8_t*& data)
{
    DhtSection sectionType;
    uint32_t sectionSize;

    sectionSize = dataToNumber<uint32_t>(data);
    sectionType = getDhtSectionType(data);
    uint8_t* const startOfSection = data;
    QJsonArray nodes;

    switch (sectionType)
    {
    case DhtSection::nodes:
        nodes = getNodeInfos(data, sectionSize);
        break;
    default:
        throw std::runtime_error("Unknown DHT section");
    }

    if ((data - startOfSection) != sectionSize)
    {
        throw std::runtime_error("Section isn't the expected size.");
    }

    return nodes;
}

QJsonArray getDht(uint8_t*& data)
{
    QJsonObject node;
    const static uint32_t dhtSectionHeader = 0x0159000d;

    auto dhtStateCookie = dataToNumber<uint32_t>(data);
    if (dhtStateCookie != dhtSectionHeader) {
        throw std::invalid_argument("Invalid DHT section header");
    }

    return getDhtSection(data);
}

QJsonObject getConferencePeer(uint8_t*& data)
{
    QJsonObject node;
    node.insert("Long term public key", readHexData(data, 32).c_str());
    node.insert("DHT public key", readHexData(data, 32).c_str());
    node.insert("Peer number", dataToNumber<uint16_t>(data));

    uint64_t timeStamp = dataToNumber<uint64_t>(data);;
    QDateTime timestamp;
    timestamp.setTime_t(timeStamp);
    node.insert("Last active timestamp", timestamp.toString(Qt::ISODate).toLocal8Bit().constData());

    int nickLen = data[0];
    node.insert("Name length", nickLen);
    data++;

    node.insert("Name", readString(data, nickLen).c_str());
    return node;
}

QJsonObject getConference(uint8_t*& data)
{
    QJsonObject node;
    node.insert("Groupchat type", data[0]);
    data += 1;
    node.insert("Groupchat id", readHexData(data, 32).c_str());
    node.insert("Message number", static_cast<int>(dataToNumber<uint32_t>(data)));
    node.insert("Lossy message number", dataToNumber<uint16_t>(data));
    node.insert("Peer number", dataToNumber<uint16_t>(data));\

    auto numPeers = dataToNumber<int>(data);
    node.insert("Number of peers", static_cast<int>(numPeers));

    int titleLen = data[0];
    node.insert("Title length", titleLen);
    data += 1;

    node.insert("Title", readString(data, titleLen).c_str());

    QJsonArray peers;
    for (int i = 0; i < numPeers; ++i) {
        peers.append(getConferencePeer(data));
    }
    node.insert("List of peers", peers);

    return node;
}

std::string getStatus(uint8_t*& data)
{
    auto status = static_cast<UserStatus>(data[0]);
    data++;
    return userStatusToString(status);
}

void addFriend(uint8_t*& data, QJsonObject& node)
{
    node.insert("Status", FriendStatusToString(static_cast<FriendStatus>(dataToNumber<uint8_t>(data))).c_str());
    node.insert("Long term public key", readHexData(data, 32).c_str());

    auto friendRequestInfoStart = data;
    static const int friendRequestMessageMaxLength = 1024;
    data += friendRequestMessageMaxLength;
    ++data; // padding

    auto infoSize = dataToNumber<uint16_t>(data, Endianness::big);
    node.insert("Size of the friend request message", infoSize);
    node.insert("Friend request message as a byte string", readString(friendRequestInfoStart, infoSize).c_str());

    auto nameStart = data;
    static const int nameMaxLength = 128;
    data += nameMaxLength;

    auto nameLength = dataToNumber<uint16_t>(data, Endianness::big);
    node.insert("Size of the name", nameLength);
    node.insert("Name as a byte string", readString(nameStart, nameLength).c_str());

    auto statusMessageStart = data;
    static const int statusMessageMaxLength = 1007;
    data += statusMessageMaxLength;
    data++; // padding

    auto statusMessageLength = dataToNumber<uint16_t>(data, Endianness::big);
    node.insert("Size of the status message", statusMessageLength);
    node.insert("Status message as a byte string", readString(statusMessageStart, statusMessageLength).c_str());

    node.insert("User status", getStatus(data).c_str());
    data += 3; // padding

    node.insert("Nospam (only used for sending a friend request)", readHexData(data, 4).c_str());

    QDateTime timestamp;
    timestamp.setTime_t(dataToNumber<uint64_t>(data, Endianness::big));
    node.insert("Last seen time", timestamp.toString(Qt::ISODate).toLocal8Bit().constData());
}

QJsonArray getFriends(uint8_t*& data, int sectionSize)
{
    uint8_t* const initialPoint = data;

    QJsonArray friends;
    while ((data - initialPoint) < sectionSize)
    {
        QJsonObject friendJson;
        addFriend(data, friendJson);
        friends.append(friendJson);
    }

    return friends;
}

QJsonArray getConferences(uint8_t*& data, int sectionSize)
{
    const uint8_t* startPos = data;
    QJsonArray groups;
    while (data - startPos < sectionSize)
    {
        groups.append(getConference(data));
    }
    return groups;
}

void parseGlobalHeader(uint8_t*& data)
{
    const static uint32_t globalHeader1 = 0x0;
    const static uint32_t globalHeader2 = 0x15ed1b1f;

    if (globalHeader1 != dataToNumber<uint32_t>(data) || globalHeader2 != dataToNumber<uint32_t>(data)) {
        throw std::runtime_error("Couldn't parse global header. Is this a tox save?");
    }
}

uint8_t* mapFile(std::string profileLocation, struct stat& fileInfo, int& fd)
{
    fd = open(profileLocation.c_str(), O_RDONLY, (mode_t)0600);

    if (fd == -1)
    {
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    fileInfo = {};

    if (fstat(fd, &fileInfo) == -1)
    {
        perror("Error getting the file size");
        exit(EXIT_FAILURE);
    }

    if (fileInfo.st_size == 0)
    {
        fprintf(stderr, "Error: File is empty, nothing to do\n");
        exit(EXIT_FAILURE);
    }

    uint8_t* const map = static_cast<uint8_t*>(mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0));
    if (map == MAP_FAILED)
    {
        close(fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }
    return map;
}

void cleanupFile(uint8_t* map, struct stat fileInfo, int fd)
{
    if (munmap(map, fileInfo.st_size) == -1)
    {
        close(fd);
        perror("Error un-mmapping the file");
    }

    close(fd);
}

struct parsedSection {
    QJsonValue json;
    SectionHeader header;
    std::string sectionName;
};

parsedSection convertSectionToJson(SectionHeader sectionHeader)
{
    const auto sectionDataStart = sectionHeader.data;

    parsedSection ret;
    ret.header = sectionHeader;
    ret.sectionName = sectionToString(sectionHeader.type);

    switch (sectionHeader.type)
    {
    case SectionType::nospamkeys:
        ret.json = getNoSpamKeys(sectionHeader.data);
        break;
    case SectionType::dht:
        ret.json = getDht(sectionHeader.data);
        break;
    case SectionType::friends:
        ret.json = getFriends(sectionHeader.data, sectionHeader.size);
        break;
    case SectionType::name:
        ret.json = readString(sectionHeader.data, sectionHeader.size).c_str();
        break;
    case SectionType::statusmessage:
        ret.json = readString(sectionHeader.data, sectionHeader.size).c_str();
        break;
    case SectionType::status:
        ret.json = getStatus(sectionHeader.data).c_str();
        break;
    case SectionType::tcpRelay:
    case SectionType::pathNode:
        ret.json = getNodeInfos(sectionHeader.data, sectionHeader.size);
        break;
    case SectionType::conferences:
        ret.json = getConferences(sectionHeader.data, sectionHeader.size);
        break;
    default :
        // unknown section
        ret.json = static_cast<int>(sectionHeader.type);
        sectionHeader.data += sectionHeader.size;
    }

    if (sectionHeader.data - sectionDataStart != sectionHeader.size)
    {
        throw std::runtime_error("Section contents didn't match section size.");
    }

    return ret;
}

void combineJson(QJsonObject &rootNode, const parsedSection &node)
{
    rootNode.insert(sectionToString(node.header.type).c_str(), node.json);
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("ToxSaveParser");

    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addPositionalArgument("profile.tox", QCoreApplication::translate("main", "Tox profile to parse."));
    parser.process(app);
    const auto args = parser.positionalArguments();

    if (args.size() != 1) {
        parser.showHelp();
    }

    const auto profileLocation = args.at(0).toStdString();
    struct stat fileInfo;
    int fd;

    // map because mmap is fun
    auto data = mapFile(profileLocation, fileInfo, fd);

    uint8_t* curPlace = data;
    try {
        parseGlobalHeader(curPlace);

        // why map-reduce parsing a 1KB file? https://www.youtube.com/watch?v=b2F-DItXtZs
        auto sections = getAllSections(curPlace);
        auto result = QtConcurrent::blockingMappedReduced<QJsonObject>(sections.begin(), sections.end(), convertSectionToJson, combineJson);
        QJsonDocument doc{result};
        std::cout << doc.toJson(QJsonDocument::Indented).toStdString() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    // TODO: make this RAII
    cleanupFile(data, fileInfo, fd);

    return 0;
}
