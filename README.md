# CushiNet

An easy-to-use C++ networking library for game development (or anything network-related).

Built with [GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets) and inspired by
[KryoNet](https://github.com/EsotericSoftware/kryonet).

## Features

The integral parts of the library are:

- [Server](include/CushiNet/Server.h): Opens a listen socket and poll group; handles all connected clients.
- [Client](include/CushiNet/Client.h): Creates a connection handle to a server.

## Why Shouldn't I Just Use GNS Directly?

CushiNet serves as a layer of abstraction over Valve's [GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets) --
simplifying the typical [client-server](https://en.wikipedia.org/wiki/Client%E2%80%93server_model) workflow.

This library references the [example_chat.cpp](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/examples/example_chat.cpp)
that Valve provides. When compared to this example, using CushiNet for servers
and clients results in SUBSTANTIALLY less code and complexity. For reference,
check out the example below.

## Example / Documentation

### Headers

Include the following headers:

```cpp
#include <CushiNet/CushiNet.h> // RunCallbacks(), Dispose()

#include <CushiNet/Server.h>   // For Server Use

#include <CushiNet/Client.h>   // For Client Use

using namespace CushiNet;      // Optional
```

### Server Listener

Create a server listener to handle connection requests and incoming messages:

```cpp
namespace {

unsigned int clientCount{ 0 };

class SListener : public ServerListener
{
  public:
    void OnConnectionRequest(Server &server, const SteamNetConnectionStatusChangedCallback_t &info, const char *&rejectReason) override
    {
        // Server size limit
        if (clientCount == 4) {
            rejectReason = "Server is full!";
            return;
        }

        // Accept the client
        if (server.AcceptClient(info.m_hConn) != k_EResultOK) {
            rejectReason = "Server is unable to accept client!";
            return;
        }

        clientCount++;
    }

    void OnMessageReceived(Server &server, const ISteamNetworkingMessage &msg) override
    {
        // 1. Read the message from the client:
        // msg.m_pData;  // data
        // msg.m_cbSize; // size of data

        // 2. If applicable, send the message to everyone else:
        // server.SendMessageToAllClients(msg.m_pData, msg.m_cbSize, k_nSteamNetworkingSend_Unreliable, msg.m_conn);
        // OR
        // server.SendMessageToAllClients(msg.m_pData, msg.m_cbSize, k_nSteamNetworkingSend_Reliable, msg.m_conn);
    }

    void OnDisconnect(Server &server, const SteamNetConnectionStatusChangedCallback_t &info) override
    {
        // Handle logic on client disconnection
        clientCount--;
    }
};

SListener sListener;

} // namespace
```

### Server

Pass the server listener instance above into a server instance:

```cpp
static Server server(sListener);
```

Call this once to start the server:

```cpp
server.Start();
```

**NOTE: The line above hosts with a cleared IP address on port 21106. If you want to host on another port, pass it as a
parameter; if on a specific IP address, use the designated `server.Start(localAddress, options, numOptions)` method.**

Call this in your game loop to update the server:

```cpp
server.Update();
```

**NOTE: Typically you want either your server or client to `RunCallbacks()`, so you should
call `Update()` as it is shown above if the server is the first network-related instance
that calls `Update()` since `RunCallbacks()` will be called globally for all `Update()`
calls that follow. See the Extras section below for more details.**

Call this to close your server:

```cpp
server.Stop();
```

**NOTE: Make sure to call this before `CushiNet::Dispose()`.**

### Client Listener

Create a client listener to handle connection callbacks and incoming messages:

```cpp
namespace {

class CListener : public ClientListener
{
  public:
    void OnConnect(Client &client, const SteamNetConnectionStatusChangedCallback_t &info) override
    {
        // Handle logic when the client successfully connects to the server
    }

    void OnMessageReceived(Client &client, const ISteamNetworkingMessage &msg) override
    {
        // Read the message from the server:
        // msg.m_pData;  // data
        // msg.m_cbSize; // size of data
    }

    void OnDisconnect(Client &client, const SteamNetConnectionStatusChangedCallback_t &info) override
    {
        // Handle logic when the client disconnects from the server
    }
};

CListener cListener;

} // namespace
```

### Client

Pass the client listener instance above into a client instance:

```cpp
static Client client(cListener);
```

Call this once to join a **local** server:

```cpp
client.JoinLocalHost();
```

**NOTE: The line above joins a locally hosted server on port 21106. If you want to join locally on another port, pass
it as a parameter; if on a specific local or remote IP address, use the two designated `client.Join()` methods.**

Call this in your game loop to update the client:

```cpp
client.Update(false);
```

**NOTE: Typically you want either your server or client to `RunCallbacks()`, so you should
call `Update(false)` as it is shown above if the server was the first network-related instance
that called `Update()` since it will call `RunCallbacks()` globally. See the Extras section below
for more details.**

Call this to close your client:

```cpp
client.Leave();
```

**NOTE: Make sure to call this before `CushiNet::Dispose()`.**

### Finishing Up

When you are done using networking completely or are shutting down your application, make sure to call:

```cpp
Dispose(); // CushiNet::Dispose();
```

to close all sockets and connections, and free resources.

### Extras

If you find the `RunCallbacks()` explanation above insufficient and/or confusing, the simplest
thing you can do instead is to pass `false` into all `Update()` calls and run:

```cpp
RunCallbacks(); // CushiNet::RunCallbacks();
```

**BEFORE CALLING EACH `Update(false)`!** <br>

For additional help and details, be sure to read all the carefully written comments above each function.
You can also have a look at Valve's [example_chat.cpp](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/examples/example_chat.cpp)
and their official [documentation](https://partner.steamgames.com/doc/api/ISteamNetworkingSockets).
