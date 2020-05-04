#include <qjsonobject.h>  // for QJsonObject
#include <stdio.h>        // for snprintf
#include <cstdint>        // for uint8_t, uint16_t
#include <stdexcept>      // for runtime_error, invalid_argument
#include <string>         // for string
#include "nodeinfo.h"
#include "utils.h"        // for dataToNumber, readHexData, Endianness, Endi...

#define CRYPTO_PUBLIC_KEY_SIZE         32

enum class TransportProtocol
{
    udp = 0,
    tcp = 1
};

enum class AddressFamily
{
    ipv4,
    ipv6
};

std::string transportProtocolToString(TransportProtocol proto)
{
    switch (proto) {
    case TransportProtocol::udp:
        return "UDP";
    case TransportProtocol::tcp:
        return "TCP";
    default:
        throw std::runtime_error("Invalid TransportProtocol passed to transportProtocolToString");
    }
}

std::string addressFamilyToString(AddressFamily fam)
{
    switch (fam) {
    case AddressFamily::ipv4:
        return "IPv4";
    case AddressFamily::ipv6:
        return "IPv6";
    default:
        throw std::runtime_error("Invalid AddressFamily passed to addressFamilyToString");
    }
}

TransportProtocol getTransportProtocol(uint8_t data)
{
    return static_cast<TransportProtocol>((data & 0x80) >> 7);
}

AddressFamily getAddressFamily(uint8_t data)
{
    auto val = data & 0x7f;
    switch (val) {
    case 2:
        return AddressFamily::ipv4;
    case 10:
        return AddressFamily::ipv6;
    default:
        throw std::runtime_error("Invalid AddressFamily when parsing nodeinfo");
    }
}

void addFamily(uint8_t*& data, AddressFamily addrFamily, QJsonObject& node)
{
    auto protocol = getTransportProtocol(data[0]);
    node.insert("Transport Protocol", transportProtocolToString(protocol).c_str());
    node.insert("Address Family", addressFamilyToString(addrFamily).c_str());
    data++;
}

void addIpAddress(uint8_t*& data, QJsonObject& node, AddressFamily addrFamily)
{
    char addressName[32+8]; // length of ipv4 address (32) + 8 colons for formatting

    switch (addrFamily) {
    case AddressFamily::ipv4:
        snprintf(addressName, sizeof(addressName), "%d.%d.%d.%d", data[0], data[1], data[2], data[3]);
        data += 4;
        break;
    case AddressFamily::ipv6:
        snprintf(addressName, sizeof(addressName), "%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X",
            data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8],
            data[9], data[10], data[11], data[12], data[13], data[14], data[15]);
        data += 16;
        break;
    default:
        throw std::invalid_argument("Invalid AddressFamily passed to addIpUnion");
    }
    node.insert("IP address", addressName);
}

void addIp(uint8_t*& data, QJsonObject& node)
{
    auto addrFamily = getAddressFamily(data[0]);
    addFamily(data, addrFamily, node);
    addIpAddress(data, node, addrFamily);
}

QJsonArray getNodeInfos(uint8_t*& data, int sectionSize)
{
    uint8_t* const initialPoint = data;
    QJsonArray nodes;
    while ((data - initialPoint) < sectionSize)
    {
        QJsonObject nodeJson;
        addIp(data, nodeJson);
        nodeJson.insert("Port Number", dataToNumber<uint16_t>(data, Endianness::big));
        nodeJson.insert("Public Key", readHexData(data, CRYPTO_PUBLIC_KEY_SIZE).c_str());
        nodes.append(nodeJson);
    }

    if ((data - initialPoint) != sectionSize)
    {
        throw std::runtime_error("Section isn't the expected size.");
    }

    return nodes;
}
