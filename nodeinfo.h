#pragma once

#include <qjsonarray.h>  // for QJsonArray
#include <cstdint>       // for uint8_t
class QJsonObject;

bool isFamilyIpv4(uint8_t family);
void addFamily(uint8_t*& data, QJsonObject& node);
void addIpUnion(uint8_t*& data, QJsonObject& node, bool isIpv4);
void addIp(uint8_t*& data, QJsonObject& node);
void addIpPort(uint8_t*& data, QJsonObject& node);
QJsonArray getNodeInfos(uint8_t*& data, int sectionSize);
