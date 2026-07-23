#include <stdexcept>

#include <CushiNet/CushiNet.h>
#include <steam/steamnetworkingsockets.h>

uint32 CushiNet::ParseIPv4Address(const std::string &ip)
{
    size_t firstDot{ ip.find_first_of('.') };
    if (firstDot == std::string::npos) {
        throw std::invalid_argument("Invalid IPv4 Address Format.");
    }

    size_t secondDot{ ip.find_first_of('.', firstDot + 1) };
    if (secondDot == std::string::npos) {
        throw std::invalid_argument("Invalid IPv4 Address Format.");
    }

    size_t thirdDot{ ip.find_first_of('.', secondDot + 1) };
    if (thirdDot == std::string::npos) {
        throw std::invalid_argument("Invalid IPv4 Address Format.");
    }

    return ((uint32)std::stoi(ip.substr(0, firstDot)) << 24) |
           ((uint32)std::stoi(ip.substr(firstDot + 1, secondDot - (firstDot + 1))) << 16) |
           ((uint32)std::stoi(ip.substr(secondDot + 1, thirdDot - (secondDot + 1))) << 8) |
           ((uint32)std::stoi(ip.substr(thirdDot + 1)));
}

uint32 CushiNet::ParseIPv4Address(uint8 first, uint8 second, uint8 third, uint8 fourth)
{
    return ((uint32)first << 24) | ((uint32)second << 16) | ((uint32)third << 8) | (uint32)fourth;
}

void CushiNet::RunCallbacks()
{
    ISteamNetworkingSockets *networkInterface{ SteamNetworkingSockets() };
    if (networkInterface) {
        networkInterface->RunCallbacks();
    }
}

void CushiNet::Dispose()
{
    GameNetworkingSockets_Kill();
}
