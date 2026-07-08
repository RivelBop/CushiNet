#pragma once

#include <unordered_map>
#include <unordered_set>

#include <steam/isteamnetworkingsockets.h>

namespace CushiNet
{

class Server;

class ServerListener
{
  public:
    virtual ~ServerListener() = default;
    virtual void OnConnectionStatusChanged(const SteamNetConnectionStatusChangedCallback_t &info) = 0;
    virtual void OnMessageReceived(const ISteamNetworkingMessage &msg) = 0;
};

class Server
{
  public:
    /**
     * Create a server with the provided `ServerListener` and `ISteamNetworkingSockets` implementation.
     *
     * The `listener` is a `nullptr` by default but setting one is highly recommended.
     *
     * The recommended default `networkInterface` implementation is provided by Valve via
     * `SteamNetworkingSockets()`.
     *
     * Throws `std::invalid_argument` if `networkInterface` is a `nullptr`.
     */
    Server(ServerListener *listener = nullptr, ISteamNetworkingSockets *networkInterface = SteamNetworkingSockets());

    /**
     * Stops the server.
     */
    ~Server();

    /**
     * Creates a server socket that listens for clients to connect and a poll group
     * to handle messages from all connections.
     *
     * Requires a `port` to host on, the `localAddress` is cleared, and a connection
     * status changed callback is configured for this server.
     *
     * Throws `std::runtime_error` if unable to create socket or poll group.
     */
    void Start(uint16 port = 21106);

    /**
     * Creates a server socket that listens for clients to connect and a poll group
     * to handle messages from all connections.
     *
     * Requires IPv4, IPv6, and port specifications provided via `localAddress`.
     *
     * For further network configuration, provide an option array `options`
     * (can be `nullptr`) and the number of options `numOptions` (can be `0`).
     *
     * WARNING: Do not alter the `k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged`
     * configuration; this is automatically set to the appropriate callback function for you.
     *
     * Throws `std::invalid_argument` if `options` is `nullptr` but `numOptions` > `0`.
     * Throws `std::runtime_error` if unable to create socket or poll group.
     */
    void Start(const SteamNetworkingIPAddr &localAddress, const SteamNetworkingConfigValue_t *options, int numOptions);

    /**
     * Closes the server socket and poll group; clears all clients.
     *
     * When closing the socket, the server is also removed from the global
     * server registry for callbacks.
     */
    void Stop();

    /**
     * Sets the `listener` to act as a callback for this server.
     *
     * `listener` can be `nullptr`, but setting it is highly recommended.
     */
    void SetListener(ServerListener *listener);

    /**
     * Polls for incoming messages and retrieves connection status changes.
     *
     * Disable `runCallbacks` if you have multiple servers or a client
     * and server running at the same time, opting to call the global
     * `RunCallbacks()` from `ISteamNetworkingSockets` to run all
     * callbacks from one location at once.
     *
     * Throws `std::runtime_error` if there is an error when receiving incoming
     * messages from poll group.
     */
    void Update(bool runCallbacks = true);

    /**
     * Accepts a client's connection; this should be called in the callback
     * when `k_ESteamNetworkingConnectionState_Connecting` is received.
     *
     * You MUST call this when accepting a client as it also adds the new
     * client into the server's client set to keep track.
     *
     * Returns:
     * - `k_EResultOK`: Connection accepted successfully.
     * - `k_EResultInvalidParam`: Handle is invalid.
     * - `k_EResultInvalidState`: Connection is not in the appropriate state.
     * - `k_EResultNoConnection`: Server is not running.
     * - `k_EResultDuplicateRequest`: Client is already in the server.
     * - `k_EResultFail`: Unable to add connection to the poll group.
     */
    EResult AcceptClient(HSteamNetConnection client);

    /**
     * Closes a client's connection, basically kicking them out.
     *
     * You MUST call this when manually removing a client as it also
     * removes the client from the server's client set to keep track.
     *
     * Returns `true` if successfully closed the connection.
     * Returns `false` if:
     * - The server isn't running.
     * - This client was not accepted into the server.
     */
    bool RemoveClient(HSteamNetConnection client);

    /**
     * Sends data to a client's connection.
     *
     * Provide a connected `client`, the necessary `data`, and
     * `networkProtocol` (`k_nSteamNetworkingSend_Unreliable`, etc.).
     *
     * Returns:
     * - `k_EResultOK`: Message successfully sent.
     * - `k_EResultInvalidParam`: Invalid connection handle, or the individual message is too big.
     * - `k_EResultInvalidState`: Connection is in an invalid state or not in the server.
     * - `k_EResultNoConnection`: Connection has ended or server is not running.
     * - `k_EResultIgnored`: Using `k_nSteamNetworkingSend_NoDelay` and message dropped since not ready to send.
     * - `k_EResultLimitExceeded`: There was already too much data queued to be sent.
     */
    EResult SendMessageToClient(HSteamNetConnection client, const void *data, uint32 dataSize, int networkProtocol) const;

    /**
     * Sends data to all connected clients.
     *
     * Provide the `data` and `networkProtocol` (`k_nSteamNetworkingSend_Unreliable`, etc.).
     * The `except` is used when the intention is to send data to all clients except one.
     *
     * Returns:
     * - `k_EResultOK`: Message successfully sent.
     * - `k_EResultInvalidParam`: Invalid connection handle, or the individual message is too big.
     * - `k_EResultInvalidState`: Connection is in an invalid state.
     * - `k_EResultNoConnection`: Connection has ended or server is not running.
     * - `k_EResultIgnored`: Using `k_nSteamNetworkingSend_NoDelay` and message dropped since not ready to send.
     * - `k_EResultLimitExceeded`: There was already too much data queued to be sent.
     */
    EResult SendMessageToAllClients(const void *data, uint32 dataSize, int networkProtocol,
                                    HSteamNetConnection except = k_HSteamNetConnection_Invalid) const;

    /**
     * Returns all connected clients.
     */
    const std::unordered_set<HSteamNetConnection> &GetClients() const;

    /**
     * Returns `true` if a server socket and poll group have been created.
     */
    bool IsRunning() const;

  private:
    ISteamNetworkingSockets *networkInterface{ nullptr };
    HSteamListenSocket socket{ k_HSteamListenSocket_Invalid };
    HSteamNetPollGroup pollGroup{ k_HSteamNetPollGroup_Invalid };
    ServerListener *listener{ nullptr };
    std::unordered_set<HSteamNetConnection> clients;

    static std::unordered_map<HSteamListenSocket, Server *> globalServerRegistry;

    /**
     * Used to alert servers of connection status changes.
     *
     * This should NEVER be called directly, this is used for
     * handling the callback system via configuration:
     *
     * `SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)connectionStatusChangedCallback)`.
     */
    static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *info);
};

} // namespace CushiNet
