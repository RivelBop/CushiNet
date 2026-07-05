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
     * Throws `std::runtime_error` if unable to create socket or poll group.
     */
    void Start(const SteamNetworkingIPAddr &localAddress, const SteamNetworkingConfigValue_t *options, int numOptions);

    /**
     * Closes the server socket and poll group.
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
     * Polls for incoming messages and connection status changes.
     */
    void Update();

    /**
     * Sends data to a client's connection.
     *
     * Provide a connected `client`, the necessary `data`, and
     * `networkProtocol` (`k_nSteamNetworkingSend_Unreliable`, etc.).
     *
     * Returns:
     * - `k_EResultInvalidParam`: Invalid connection handle, or the individual message is too big.
     * - `k_EResultInvalidState`: Connection is in an invalid state.
     * - `k_EResultNoConnection`: Connection has ended.
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
     * - `k_EResultInvalidParam`: Invalid connection handle, or the individual message is too big.
     * - `k_EResultInvalidState`: Connection is in an invalid state.
     * - `k_EResultNoConnection`: Connection has ended.
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

    /**
     * Used to alert servers of connection status changes.
     *
     * This should NEVER be called directly, this is useful when implementing
     * custom server configurations, allowing the user to utilize the built-in
     * callback system by calling:
     *
     * `SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)connectionStatusChangedCallback)`.
     */
    static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *info);

  private:
    ISteamNetworkingSockets *networkInterface{ nullptr };
    HSteamListenSocket socket{ k_HSteamListenSocket_Invalid };
    HSteamNetPollGroup pollGroup{ k_HSteamNetPollGroup_Invalid };
    ServerListener *listener{ nullptr };
    std::unordered_set<HSteamNetConnection> clients;

    static std::unordered_map<HSteamListenSocket, Server *> globalServerRegistry;
};

} // namespace CushiNet
