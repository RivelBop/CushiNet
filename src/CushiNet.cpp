#include <CushiNet/CushiNet.h>
#include <steam/steamnetworkingsockets.h>

void CushiNet::Dispose()
{
    GameNetworkingSockets_Kill();
}
