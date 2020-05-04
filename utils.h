#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

enum class FriendStatus {
    notAFriend,
    added,
    requestSent,
    confirmed,
    online,
};

enum class UserStatus {
    online = 0,
    away,
    busy,
};

enum class Endianness {
    big,
    little
};

template <typename T>
T dataToNumber(uint8_t*& data, Endianness order = Endianness::little)
{
    T val = 0;
    int size = sizeof(T);
    // TODO: don't assume host is little endian
    if (order == Endianness::little) {
        for (int i = 0; i < size; ++i) {
            val |= data[i] << 8*i;
        }
    } else {
        for (int i = 0; i < size; ++i) {
            val |= data[i] << 8*(size-i-1);
        }
    }
    data += size;
    return val;
}

std::string readString(uint8_t*& data, size_t length);

std::string readHexData(uint8_t*& data, int size);
std::string userStatusToString(UserStatus status);
std::string FriendStatusToString(FriendStatus status);
