#include <stdexcept>

#include <CushiNet/CushiNet.h>
#include <steam/steamnetworkingsockets.h>

uint32 CushiNet::ParseIPv4Address(const std::string &ip)
{
    size_t firstDot{ ip.find_first_of('.') };
    if (firstDot == std::string::npos) {
        throw std::invalid_argument("Missing First IPv4 Dot.");
    }

    uint32 ip1 { (uint32) std::stoi(ip.substr(0, firstDot)) };
    if (ip1 < 0 || ip1 > 255) {
    	throw std::invalid_argument("First Portion of IPv4 Must Be 0-255.");
    }

    size_t secondDot{ ip.find_first_of('.', firstDot + 1) };
    if (secondDot == std::string::npos) {
        throw std::invalid_argument("Missing Second IPv4 Dot.");
    }

    uint32 ip2 { (uint32) std::stoi(ip.substr(firstDot + 1, secondDot - (firstDot + 1))) };
    if (ip2 < 0 || ip2 > 255) {
    	throw std::invalid_argument("Second Portion of IPv4 Must Be 0-255.");
    }

    size_t thirdDot{ ip.find_first_of('.', secondDot + 1) };
    if (thirdDot == std::string::npos) {
        throw std::invalid_argument("Missing Third IPv4 Dot.");
    }

    uint32 ip3 { (uint32) std::stoi(ip.substr(secondDot + 1, thirdDot - (secondDot + 1))) };
    if (ip3 < 0 || ip3 > 255) {
    	throw std::invalid_argument("Third Portion of IPv4 Must Be 0-255.");
    }

	uint32 ip4 { (uint32) std::stoi(ip.substr(thirdDot + 1)) };
	if (ip4 < 0 || ip4 > 255) {
		throw std::invalid_argument("Fourth Portion of IPv4 Must Be 0-255.");
	}

    return (ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4;
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
