# Server Design

## Networking model

Design is based on this article: https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking

## Development process

In order to properly develop and appreciate each feature adds, iterative builds will be created:

1. **Naive**: Deltas are sent every frame from the server to the client. No interpolation, lag compensation or client prediction
1. **Tick Server**: Deltas are sent in batches on each *tick* instead of every frame. No additional features
1. **Interpolation**: Deltas are sent every tick and interpolation is applied on the client when displaying the characters.
1. **Lag Compesation**: Raycasting is checked against player controlled hitboxes. 
1. **Input prediction**: Clients immediately move their character, but it's position will be always checked against server data.

## Packet Protocol

Describes which packets and in what order are interchanged during the *handshake* and during the game.

### Handshake
1. **Client Request**: Client request connection to the game server. Server acks or rejects. A client version could be sent here for example.
1. **Server Configuration**: Servers sends a packet with relevant configuration data, such as interpolation time, tick rate, server time, etc.

### Match
1. **Server Sends Game Start Event**: This will allow the players to move.
2. **Client - Move Command**: They are batched, depending on framerate and tickrate.
3. **Server sends movement deltas**: Server sends back to every client the movement deltas after the simluation each tick.

## Testing

In order to simulate a laggy enviroment, a proxy made in Rust will be used to introduce noice, increase response time and randomly drop/duplicate packets.

During these tests, simple player movement will be tested, with no collisions involved. Initially the map to use will be accorded beforehand. Downloading the map data is a problem to be solved in a later stage.

## How's the Map data sent to the clients ?

There are a few options available:

1. **External**: Clients already have the map downloaded. A database file is kept to index by map name/id to a given file path to be loaded once the game starts.
2. **Download from Game Server**: Clients retrieve the map file and texture from the game server, whichs need to have it available beforehand.
3. **Download from MatchMaking Server**: Game creator uploads the map to matchmaking server. Clients download the data from this server through TCP.
