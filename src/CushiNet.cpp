#include <CushiNet/CushiNet.h>
#include <steam/steamnetworkingsockets.h>

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
