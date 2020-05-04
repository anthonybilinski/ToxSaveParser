#include "utils.h"
#include <stdio.h>  // for sprintf
#include <cstdint>  // for uint8_t
#include <string>   // for string
#include <vector>   // for vector
#include <stdexcept>

std::string readString(uint8_t*& data, size_t length)
{
    auto string = std::string(reinterpret_cast<char*>(data), length);
    data += length;
    return string;
}

std::string readHexData(uint8_t*& data, int size)
{
    std::vector<char> hexString(size*2, 0);

    for (int i = 0; i < size; ++i)
    {
        sprintf(hexString.data()+i*2, "%02X", data[i]);
    }
    data += size;

    return {hexString.begin(), hexString.end()};
}

std::string userStatusToString(UserStatus status)
{
    switch (status)
    {
    case UserStatus::online:
        return "online";
    case UserStatus::away:
        return "away";
    case UserStatus::busy:
        return "busy";
    default:
        throw std::runtime_error("Invalid user status");
    }
}

std::string FriendStatusToString(FriendStatus status)
{
    switch (status)
    {
    case FriendStatus::notAFriend:
        return "Not a friend";
    case FriendStatus::added:
        return "Friend added";
    case FriendStatus::requestSent:
        return "Friend request sent";
    case FriendStatus::confirmed:
        return "Confirmed friend";
    case FriendStatus::online:
        return "Friend online";
    default:
        throw std::runtime_error("Invalid friend status value");
    }
}
