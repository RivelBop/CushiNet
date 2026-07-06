#include <stdexcept>

#include <CushiNet/Server.h>

namespace CushiNet
{

Server::Server(ServerListener *listener, ISteamNetworkingSockets *networkInterface)
    : listener(listener), networkInterface(networkInterface)
{
    if (!networkInterface) {
        throw std::invalid_argument("Network interface cannot be null.");
    }
}

Server::~Server()
{
    Stop();
}

void Server::Start(uint16 port)
{
    // Prevent starting the same server again
    if (IsRunning()) {
        return;
    }

    // Set cleared IP with port
    SteamNetworkingIPAddr serverLocalAddr;
    serverLocalAddr.Clear();
    serverLocalAddr.m_port = port;

    // Set connection status changed callback
    SteamNetworkingConfigValue_t opt;
    opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)ConnectionStatusChangedCallback);

    // Create server socket
    socket = networkInterface->CreateListenSocketIP(serverLocalAddr, 1, &opt);
    if (socket == k_HSteamListenSocket_Invalid) {
        throw std::runtime_error("Unable to create server socket.");
    }

    // Create poll group to handle messages from multiple client connections
    pollGroup = networkInterface->CreatePollGroup();
    if (pollGroup == k_HSteamNetPollGroup_Invalid) {
        networkInterface->CloseListenSocket(socket);
        socket = k_HSteamListenSocket_Invalid;
        throw std::runtime_error("Unable to create server poll group.");
    }

    // Register server to global map (for proper callback functionality)
    globalServerRegistry[socket] = this;
}

void Server::Start(const SteamNetworkingIPAddr &localAddress, const SteamNetworkingConfigValue_t *options, int numOptions)
{
    // Prevent starting the same server again
    if (IsRunning()) {
        return;
    }

    // Create server socket
    socket = networkInterface->CreateListenSocketIP(localAddress, numOptions, options);
    if (socket == k_HSteamListenSocket_Invalid) {
        throw std::runtime_error("Unable to create server socket.");
    }

    // Create poll group to handle messages from multiple client connections
    pollGroup = networkInterface->CreatePollGroup();
    if (pollGroup == k_HSteamNetPollGroup_Invalid) {
        networkInterface->CloseListenSocket(socket);
        socket = k_HSteamListenSocket_Invalid;
        throw std::runtime_error("Unable to create server poll group.");
    }

    // Register server to global map (for optional callback functionality)
    globalServerRegistry[socket] = this;
}

void Server::Stop()
{
    // Destroy server socket
    if (socket != k_HSteamListenSocket_Invalid) {
        globalServerRegistry.erase(socket);
        networkInterface->CloseListenSocket(socket);
        socket = k_HSteamListenSocket_Invalid;
    }

    // Destroy poll group
    if (pollGroup != k_HSteamNetPollGroup_Invalid) {
        networkInterface->DestroyPollGroup(pollGroup);
        pollGroup = k_HSteamNetPollGroup_Invalid;
    }

    // All connected clients are no longer valid
    clients.clear();
}

void Server::SetListener(ServerListener *listener)
{
    this->listener = listener;
}

void Server::Update()
{
    // Make sure the server is running
    if (!IsRunning()) {
        return;
    }

    // Receive incoming messages
    ISteamNetworkingMessage *incomingMsg{ nullptr };
    int numMsgs{ networkInterface->ReceiveMessagesOnPollGroup(pollGroup, &incomingMsg, 1) };
    while (numMsgs == 1 && incomingMsg) {
        if (listener) {
            listener->OnMessageReceived(*incomingMsg);
        }
        incomingMsg->Release();
        numMsgs = networkInterface->ReceiveMessagesOnPollGroup(pollGroup, &incomingMsg, 1);
    }

    // Critical Error: Unable to receive incoming messages
    if (numMsgs < 0) {
        Stop();
        throw std::runtime_error("Unable to receive messages on server poll group. Stopping server.");
    }

    // Receive connection state changes
    networkInterface->RunCallbacks();
}

EResult Server::SendMessageToClient(HSteamNetConnection client, const void *data, uint32 dataSize, int networkProtocol) const
{
    // Ensures the server is active
    if (!IsRunning()) {
        return k_EResultNoConnection;
    }

    // Ensures client is a part of the server
    if (clients.find(client) == clients.end()) {
        return k_EResultInvalidState;
    }

    return networkInterface->SendMessageToConnection(client, data, dataSize, networkProtocol, nullptr);
}

EResult Server::SendMessageToAllClients(const void *data, uint32 dataSize, int networkProtocol, HSteamNetConnection except) const
{
    if (!IsRunning()) {
        return k_EResultNoConnection;
    }

    EResult result{ k_EResultOK };
    for (auto client : clients) {
        if (client == except) {
            continue;
        }

        EResult currentResult{ networkInterface->SendMessageToConnection(client, data, dataSize, networkProtocol, nullptr) };
        if (currentResult != k_EResultOK) {
            result = currentResult;
        }
    }
    return result;
}

const std::unordered_set<HSteamNetConnection> &Server::GetClients() const
{
    return clients;
}

bool Server::IsRunning() const
{
    return socket != k_HSteamListenSocket_Invalid && pollGroup != k_HSteamNetPollGroup_Invalid;
}

void Server::ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *info)
{
    // This null-check was not in Valve's example, it is added here just in case
    if (!info) {
        return;
    }

    auto iterator{ globalServerRegistry.find(info->m_info.m_hListenSocket) };
    if (iterator != globalServerRegistry.end()) {
        Server *server{ iterator->second };

        // Keep track of all connected clients;
        // Useful when sending messages to all clients
        switch (info->m_info.m_eState) {

        // Handle disconnecting clients
        case k_ESteamNetworkingConnectionState_None:
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            server->clients.erase(info->m_hConn);
            break;

        // Handle connecting clients
        case k_ESteamNetworkingConnectionState_Connected:
            server->clients.insert(info->m_hConn);
            break;

        // Silences -Wswitch
        default:
            break;
        }

        // Using the provided listen socket information, we can determine
        // which server listener to call from the global server registry
        ServerListener *listener{ server->listener };
        if (listener) {
            listener->OnConnectionStatusChanged(*info);
        }

        switch (info->m_info.m_eState) {

        // Clean up the connection
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            server->networkInterface->CloseConnection(info->m_hConn, 0, nullptr, false);
            break;

        // Silences -Wswitch
        default:
            break;
        }
    }
}

} // namespace CushiNet
