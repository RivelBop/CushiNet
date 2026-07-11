#pragma once

namespace CushiNet
{

/**
 * Close all connections and listen sockets, and free all resources
 * from Valve's GameNetworkingSockets.
 *
 * This should be called at the very end of your application's
 * life cycle to prevent interference with active Clients and
 * Servers.
 */
void Dispose();

} // namespace CushiNet
