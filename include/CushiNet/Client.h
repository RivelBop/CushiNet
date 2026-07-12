#pragma once

#include <unordered_map>

#include <steam/isteamnetworkingsockets.h>

namespace CushiNet
{

class Client;

class ClientListener
{
  public:
    virtual ~ClientListener() = default;

    /**
     * Called when the client's connection is accepted by the server, aka when
     * `info.m_info.m_eState` is `k_ESteamNetworkingConnectionState_Connected`.
     */
    virtual void OnConnect(Client &client, const SteamNetConnectionStatusChangedCallback_t &info) = 0;

    /**
     * Called when a message is received from the server.
     *
     * You should read the message and possibly send something back via
     * `SendMessage(data, dataSize, networkProtocol)`.
     */
    virtual void OnMessageReceived(Client &client, const ISteamNetworkingMessage &msg) = 0;

    /**
     * Called when the client disconnects from the server, aka when
     * `info.m_info.m_eState` is either `k_ESteamNetworkingConnectionState_ClosedByPeer`
     * or `k_ESteamNetworkingConnectionState_ProblemDetectedLocally`.
     *
     * The server's connection handle has already been closed so you cannot
     * send a message to the server.
     *
     * `OnDisconnect()` will NOT be called if you manually/forcefully exit the
     * server via `Leave()`. In this case, you should handle the disconnect
     * logic right after instead of waiting for `RunCallbacks()`.
     */
    virtual void OnDisconnect(Client &client, const SteamNetConnectionStatusChangedCallback_t &info) = 0;
};

class Client
{
  public:
    /**
     * Create a client by providing an optional `ClientListener`.
     *
     * The `listener` is a `nullptr` by default but setting one is highly recommended.
     *
     * The `networkInterface` implementation is provided by Valve via
     * `SteamNetworkingSockets()`.
     *
     * Throws `std::runtime_error` if unable to initialize GameNetworkingSockets.
     */
    Client(ClientListener *listener = nullptr);

    /**
     * Create a client with the provided `ISteamNetworkingSockets` implementation and
     * an optional `ClientListener`.
     *
     * The `listener` is a `nullptr` by default but setting one is highly recommended.
     *
     * Throws `std::invalid_argument` if `networkInterface` is a `nullptr`.
     */
    Client(ISteamNetworkingSockets *networkInterface, ClientListener *listener = nullptr);

    /**
     * Disconnects the client from the server by calling `Leave()`.
     */
    ~Client();

    /**
     * Creates a connection to a server by calling `SetIPv6LocalHost()`
     * with the provided port (defaults to `21106`) on the `serverAddress`
     * and passing it into the Client's
     * `Join(const SteamNetworkingIPAddr &serverAddress)`.
     *
     * Throws `std::runtime_error` if unable to connect to server.
     */
    void JoinLocalHost(uint16 port = 21106);

    /**
     * Creates a connection to a server using the provided IPv4, IPv6,
     * and port specifications in `serverAddress`. If this client is
     * already in a server, this will leave that server and attempt to
     * join the provided one.
     *
     * Throws `std::runtime_error` if unable to connect to server.
     */
    void Join(const SteamNetworkingIPAddr &serverAddress);

    /**
     * Creates a connection to a server using the provided IPv4, IPv6,
     * and port specifications in `serverAddress` and advanced network
     * configurations in `options`. If this client is already in a server,
     * this will leave that server and attempt to join the provided one.
     *
     * The configuration array `options` can be a `nullptr` and the
     * number of options `numOptions` can be `0`.
     *
     * WARNING: Do not alter the `k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged`
     * configuration; this is automatically set to the appropriate callback function for you.
     *
     * Throws `std::invalid_argument` if `options` is `nullptr` but `numOptions` > `0`.
     * Throws `std::runtime_error` if unable to connect to server.
     */
    void Join(const SteamNetworkingIPAddr &serverAddress, const SteamNetworkingConfigValue_t *options, int numOptions);

    /**
     * Closes the client's connection to the server.
     *
     * When closing the connection, the client is also removed from the global
     * client registry for callbacks.
     */
    void Leave();

    /**
     * Sets the `listener` to act as a callback for this client.
     *
     * `listener` can be `nullptr`, but setting it is highly recommended.
     */
    void SetListener(ClientListener *listener);

    /**
     * Polls for incoming messages and retrieves connection status changes.
     *
     * Disable `runCallbacks` if you have a client and server running at the
     * same time, opting to call the global `RunCallbacks()` from
     * `ISteamNetworkingSockets` to run all callbacks from one location at once.
     *
     * Throws `std::runtime_error` if there is an error when receiving incoming
     * messages from the server.
     */
    void Update(bool runCallbacks = true);

    /**
     * Sends data to the server. Provide the necessary `data`, and `networkProtocol`
     * (`k_nSteamNetworkingSend_Unreliable`, etc.).
     *
     * Returns:
     * - `k_EResultOK`: Message successfully sent.
     * - `k_EResultInvalidParam`: Invalid connection handle, or the individual message is too big.
     * - `k_EResultInvalidState`: Connection is in an invalid state.
     * - `k_EResultNoConnection`: Not connected to server.
     * - `k_EResultIgnored`: Using `k_nSteamNetworkingSend_NoDelay` and message dropped since not ready to send.
     * - `k_EResultLimitExceeded`: There was already too much data queued to be sent.
     */
    EResult SendMessage(const void *data, uint32 dataSize, int networkProtocol) const;

    /**
     * Returns `true` if a `HSteamNetConnection` has been established with a server.
     */
    [[nodiscard]] bool IsConnected() const;

  private:
    ISteamNetworkingSockets *networkInterface{ nullptr };
    HSteamNetConnection connection{ k_HSteamNetConnection_Invalid };
    ClientListener *listener{ nullptr };

    static std::unordered_map<HSteamNetConnection, Client *> globalClientRegistry;

    /**
     * Used to alert clients of connection status changes.
     *
     * This should NEVER be called directly, this is used for
     * handling the callback system via configuration:
     *
     * `SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void *)connectionStatusChangedCallback)`.
     */
    static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *info);
};

} // namespace CushiNet
