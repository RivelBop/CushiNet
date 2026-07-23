#pragma once

#include <string>

using uint8 = unsigned char;
using uint32 = unsigned int;

namespace CushiNet
{

/**
 * Pass a `std::string` in IPv4 format:
 * `[0-255].[0-255].[0-255].[0-255]`
 *
 * Returns a value representing your IPv4 address that you can pass into:
 * `SteamNetworkingIPAddr::SetIPv4()`.
 *
 * Throws `std::invalid_argument` if the IPv4 address is invalidly formatted.
 */
uint32 ParseIPv4Address(const std::string &ip);

/**
 * Pass 4 octets representing each IPv4 address segment.
 *
 * Returns a value representing your IPv4 address that you can pass into:
 * `SteamNetworkingIPAddr::SetIPv4()`.
 */
uint32 ParseIPv4Address(uint8 first, uint8 second, uint8 third, uint8 fourth);

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
