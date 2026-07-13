#pragma once

namespace CushiNet
{

/**
 * Runs the callbacks from Valve's `SteamNetworkingSockets()`.
 *
 * Use this if you prefer to run all callbacks from one place
 * BEFORE calling the `Update(false)` methods of your `Server`
 * and/or `Client`.
 */
void RunCallbacks();

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
