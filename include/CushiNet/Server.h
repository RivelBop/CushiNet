#pragma once

#include <steam/isteamnetworkingsockets.h>

namespace CushiNet
{

class Server
{
  public:
    /**
     * Create a server with the provided SteamNetworkingSockets implementation.
     *
     * The recommended default implementation is provided by Valve via
     * `SteamNetworkingSockets()`.
     */
    Server(ISteamNetworkingSockets *networkInterface = SteamNetworkingSockets());

    /**
     * Creates a server socket that listens for clients to connect and a poll group
     * to handle messages from all connections.
     *
     * Requires a `port` to host on, the `localAddress` is cleared, and default
     * network configurations are used.
     *
     * Returns true if the server socket and poll group were successfully created.
     */
    [[nodiscard]] bool start(uint16 port = 21106);

    /**
     * Creates a server socket that listens for clients to connect and a poll group
     * to handle messages from all connections.
     *
     * Requires ipv4, ipv6, and port specifications provided via `localAddress`.
     *
     * For further network configuration, provide the number of options configured via
     * `nOptions` (can be 0) and an option array `pOptions` (can be `nullptr`).
     *
     * Returns true if the server socket and poll group were successfully created.
     */
    [[nodiscard]] bool start(const SteamNetworkingIPAddr &localAddress, int nOptions,
                             const SteamNetworkingConfigValue_t *pOptions);

  private:
    ISteamNetworkingSockets *networkInterface{ nullptr };
    HSteamListenSocket socket{ k_HSteamListenSocket_Invalid };
    HSteamNetPollGroup pollGroup{ k_HSteamNetPollGroup_Invalid };
};

} // namespace CushiNet
