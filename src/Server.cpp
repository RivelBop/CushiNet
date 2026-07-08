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

    // Expand options to include the callbacks
    numOptions = (numOptions < 0) ? 0 : numOptions;
    if (numOptions > 0 && !options) {
        throw std::invalid_argument("Options array cannot be null if numOptions > 0.");
    }
    SteamNetworkingConfigValue_t *expandedOptions{ new SteamNetworkingConfigValue_t[numOptions + 1] };
    std::copy(options, options + numOptions, expandedOptions);
    expandedOptions[numOptions] = {};
    expandedOptions[numOptions].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)ConnectionStatusChangedCallback);

    // Create server socket
    socket = networkInterface->CreateListenSocketIP(localAddress, numOptions + 1, expandedOptions);
    delete[] expandedOptions;
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
    // All connected clients are no longer valid
    if (IsRunning()) {
        for (auto client : clients) {
            networkInterface->CloseConnection(client, 0, "Server Shutdown", true);
        }
    }
    clients.clear();

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

EResult Server::AcceptClient(HSteamNetConnection client)
{
    // Ensures the server is running
    if (!IsRunning()) {
        return k_EResultNoConnection;
    }

    // This should be a new client
    if (clients.find(client) != clients.end()) {
        return k_EResultDuplicateRequest;
    }

    // Accept the client's connection
    EResult result{ networkInterface->AcceptConnection(client) };
    if (result != k_EResultOK) {
        networkInterface->CloseConnection(client, 0, "Server Failed to Accept", false);
        return result;
    }

    // Add the client to the poll group to receive messages
    if (!networkInterface->SetConnectionPollGroup(client, pollGroup)) {
        networkInterface->CloseConnection(client, 0, "Server Failed Adding to Poll Group", false);
        return k_EResultFail;
    }

    // Keep track of the client
    clients.insert(client);

    return k_EResultOK;
}

bool Server::RemoveClient(HSteamNetConnection client)
{
    if (!IsRunning() || clients.find(client) == clients.end()) {
        return false;
    }

    networkInterface->CloseConnection(client, 0, "Server Removed Client", true);
    clients.erase(client);
    return true;
}

EResult Server::SendMessageToClient(HSteamNetConnection client, const void *data, uint32 dataSize, int networkProtocol) const
{
    // Ensures the server is running
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

        // Ensure the server is running
        if (!server || !server->IsRunning()) {
            return;
        }

        switch (info->m_info.m_eState) {

        // Handle disconnecting clients
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            server->networkInterface->CloseConnection(info->m_hConn, 0, nullptr, false);
            server->clients.erase(info->m_hConn);
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
    }
}

} // namespace CushiNet
