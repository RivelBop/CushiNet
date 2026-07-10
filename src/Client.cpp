#include <stdexcept>
#include <vector>

#include <CushiNet/Client.h>

namespace CushiNet
{

Client::Client(ClientListener *listener, ISteamNetworkingSockets *networkInterface)
    : listener(listener), networkInterface(networkInterface)
{
    if (!networkInterface) {
        throw std::invalid_argument("Network interface cannot be null.");
    }
}

Client::~Client()
{
    Leave();
}

void Client::Join(const SteamNetworkingIPAddr &serverAddress)
{
    // If already in server, leave it
    Leave();

    // Set connection status changed callback
    SteamNetworkingConfigValue_t opt;
    opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)ConnectionStatusChangedCallback);

    // Connect to the server
    connection = networkInterface->ConnectByIPAddress(serverAddress, 1, &opt);
    if (connection == k_HSteamNetConnection_Invalid) {
        throw std::runtime_error("Client failed to connect to server.");
    }

    // Register client to global map (for proper callback functionality)
    globalClientRegistry[connection] = this;
}

void Client::Join(const SteamNetworkingIPAddr &serverAddress, const SteamNetworkingConfigValue_t *options, int numOptions)
{
    // If already in server, leave it
    Leave();

    // Expand options to include the callbacks
    numOptions = (numOptions < 0) ? 0 : numOptions;
    if (numOptions > 0 && !options) {
        throw std::invalid_argument("Options array cannot be null if numOptions > 0.");
    }
    std::vector<SteamNetworkingConfigValue_t> expandedOptions(options, options + numOptions);
    SteamNetworkingConfigValue_t callbackOpt;
    callbackOpt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)ConnectionStatusChangedCallback);
    expandedOptions.push_back(callbackOpt);

    // Connect to the server
    connection = networkInterface->ConnectByIPAddress(serverAddress, expandedOptions.size(), expandedOptions.data());
    if (connection == k_HSteamNetConnection_Invalid) {
        throw std::runtime_error("Client failed to connect to server.");
    }

    // Register client to global map (for proper callback functionality)
    globalClientRegistry[connection] = this;
}

void Client::Leave()
{
    if (IsConnected()) {
        globalClientRegistry.erase(connection);
        networkInterface->CloseConnection(connection, 0, "Client Left", true);
        connection = k_HSteamNetConnection_Invalid;
    }
}

void Client::SetListener(ClientListener *listener)
{
    this->listener = listener;
}

void Client::Update(bool runCallbacks)
{
    // Ensure client is connected to a server
    if (!IsConnected()) {
        return;
    }

    // Receive incoming messages
    constexpr int maxMsgs{ 16 };
    ISteamNetworkingMessage *incomingMsgs[maxMsgs];
    int numMsgs{ networkInterface->ReceiveMessagesOnConnection(connection, incomingMsgs, maxMsgs) };
    while (numMsgs > 0) {
        for (int i{ 0 }; i < numMsgs; i++) {
            if (listener) {
                listener->OnMessageReceived(*incomingMsgs[i]);
            }
            incomingMsgs[i]->Release();
        }
        numMsgs = networkInterface->ReceiveMessagesOnConnection(connection, incomingMsgs, maxMsgs);
    }

    // Critical Error: Unable to receive incoming messages
    if (numMsgs < 0) {
        Leave();
        throw std::runtime_error("Unable to receive messages from server. Disconnecting from server.");
    }

    // Receive connection state changes
    if (runCallbacks) {
        networkInterface->RunCallbacks();
    }
}

EResult Client::SendMessage(const void *data, uint32 dataSize, int networkProtocol) const
{
    // Ensure client is connected to a server
    if (!IsConnected()) {
        return k_EResultNoConnection;
    }

    return networkInterface->SendMessageToConnection(connection, data, dataSize, networkProtocol, nullptr);
}

bool Client::IsConnected() const
{
    return connection != k_HSteamNetConnection_Invalid;
}

void Client::ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *info)
{
    // This null-check was not in Valve's example, it is added here just in case
    if (!info) {
        return;
    }

    auto iterator{ globalClientRegistry.find(info->m_hConn) };
    if (iterator != globalClientRegistry.end()) {
        Client *client{ iterator->second };

        // Ensure the client is connected to a server
        if (!client || !client->IsConnected()) {
            return;
        }

        // Using the provided connection information, we can determine
        // which client listener to call from the global client registry
        ClientListener *listener{ client->listener };

        switch (info->m_info.m_eState) {

        // Handle when connected to server
        case k_ESteamNetworkingConnectionState_Connected:
        {
            if (listener) {
                listener->OnConnect(*info);
            }
            break;
        }

        // Handle disconnecting from server
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            globalClientRegistry.erase(client->connection);

            client->networkInterface->CloseConnection(client->connection, 0, "Client Disconnected", false);
            client->connection = k_HSteamNetConnection_Invalid;

            if (listener) {
                listener->OnDisconnect(*info);
            }
            break;
        }

        // Silences -Wswitch
        default:
            break;
        }
    }
}

} // namespace CushiNet
