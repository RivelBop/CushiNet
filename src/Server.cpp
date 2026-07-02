#include <stdexcept>

#include <CushiNet/Server.h>

namespace CushiNet
{

Server::Server(ISteamNetworkingSockets *networkInterface)
    : networkInterface(networkInterface)
{
    if (!networkInterface) {
        throw std::invalid_argument("Network interface cannot be null.");
    }
}

bool Server::start(uint16 port)
{
    // Prevent starting the same server again
    if (socket != k_HSteamListenSocket_Invalid || pollGroup != k_HSteamNetPollGroup_Invalid) {
        return true;
    }

    // Set cleared IP with port
    SteamNetworkingIPAddr serverLocalAddr;
    serverLocalAddr.Clear();
    serverLocalAddr.m_port = port;

    // Create server socket
    socket = networkInterface->CreateListenSocketIP(serverLocalAddr, 0, nullptr);
    if (socket == k_HSteamListenSocket_Invalid) {
        return false;
    }

    // Create poll group to handle messages from multiple client connections
    pollGroup = networkInterface->CreatePollGroup();
    if (pollGroup == k_HSteamNetPollGroup_Invalid) {
        networkInterface->CloseListenSocket(socket);
        return false;
    }

    return true;
}

bool Server::start(const SteamNetworkingIPAddr &localAddress, int nOptions,
                   const SteamNetworkingConfigValue_t *pOptions)
{
    // Prevent starting the same server again
    if (socket != k_HSteamListenSocket_Invalid || pollGroup != k_HSteamNetPollGroup_Invalid) {
        return true;
    }

    // Create server socket
    socket = networkInterface->CreateListenSocketIP(localAddress, nOptions, pOptions);
    if (socket == k_HSteamListenSocket_Invalid) {
        return false;
    }

    // Create poll group to handle messages from multiple client connections
    pollGroup = networkInterface->CreatePollGroup();
    if (pollGroup == k_HSteamNetPollGroup_Invalid) {
        networkInterface->CloseListenSocket(socket);
        return false;
    }

    return true;
}

void Server::stop()
{
    // Destroy server socket
    if (socket != k_HSteamListenSocket_Invalid) {
        networkInterface->CloseListenSocket(socket);
    }

    // Destroy poll group
    if (pollGroup != k_HSteamNetPollGroup_Invalid) {
        networkInterface->DestroyPollGroup(pollGroup);
    }
}

} // namespace CushiNet
