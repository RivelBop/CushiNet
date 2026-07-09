#pragma once

#include <unordered_map>

#include <steam/isteamnetworkingsockets.h>

namespace CushiNet
{

class ClientListener
{
  public:
    virtual ~ClientListener() = default;

    virtual void OnConnect(const SteamNetConnectionStatusChangedCallback_t &info) = 0;

    virtual void OnMessageReceived(const ISteamNetworkingMessage &msg) = 0;

    virtual void OnDisconnect(const SteamNetConnectionStatusChangedCallback_t &info) = 0;
};

class Client
{
  public:
    Client(ClientListener *listener = nullptr, ISteamNetworkingSockets *networkInterface = SteamNetworkingSockets());

    ~Client();

    void Join(const SteamNetworkingIPAddr &serverAddress);

    void Join(const SteamNetworkingIPAddr &serverAddress, const SteamNetworkingConfigValue_t *options, int numOptions);

    void Leave();

    void SetListener(ClientListener *listener);

    void Update(bool runCallbacks = true);

    EResult SendMessage(const void *data, uint32 dataSize, int networkProtocol) const;

    bool IsConnected() const;

  private:
    ISteamNetworkingSockets *networkInterface{ nullptr };
    HSteamNetConnection connection{ k_HSteamNetConnection_Invalid };
    ClientListener *listener{ nullptr };

    static std::unordered_map<HSteamNetConnection, Client *> globalClientRegistry;

    static void ConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *info);
};

} // namespace CushiNet
