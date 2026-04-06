# Raw C++ Multiplayer Pong

A completely from-scratch, online multiplayer Pong game built using native C++20, raw POSIX TCP sockets, and the Raylib graphics library. 

This project was built as a deep-dive into low-level network programming, dealing directly with TCP stream fragmentation, game loops, authoritative servers, and networking architecture without relying on high-level engines like Unity or Unreal.

## Architecture

This game uses an **Authoritative Server Architecture**:
- **The Server (`PongServer`)**: Runs the physics engine at 60 FPS. It maintains the absolute "truth" of the game state (ball position, velocities, score, paddles). It receives inputs from clients and broadcasts the updated world state 60 times a second.
- **The Client (`PongClient`)**: Acts as a "dumb terminal". It collects keyboard input (`W` and `S` keys) and streams it to the server. It receives the `GameState` packet from the server and uses Raylib to render it to the screen.

### Key Networking Features Overcome
* **TCP Stream Fragmentation:** Because TCP is a continuous byte-stream (not a message queue), sending 60 packets a second causes packets to combine or tear in transit. The server implements a robust vector-based framing buffer to stitch partial bytes back together, absolutely guaranteeing packet alignment.
* **Momentum Transfer:** The paddle velocity transfers to the ball upon collision, introducing a dynamic curve to rallies. Clamp limits prevent physics explosion.
* **Room Management:** Players perform a handshake to create or join 4-character room codes. The server natively tracks connections and cleans up disconnected sockets via system polling (`poll()`).

---

## Requirements & Dependencies

To compile and run this project, you need:
* A Linux environment (or Windows Subsystem for Linux / WSL)
* `g++` (Compiler supporting C++20)
* **[Raylib](https://github.com/raysan5/raylib)** installed on your system (Client only)

---

## ??How to Compile??

Because the Server and Client are decoupled, they must be compiled separately. Open your terminal at the project root folder.

### 1. Compile the Server
The server only uses standard POSIX network libraries (no graphics).
```bash
g++ -Wall -std=c++20 \
    Server/src/main.cpp \
    Server/src/TCPServer.cpp \
    Server/src/RoomManager.cpp \
    Game/src/GameSessions.cpp \
    -o PongServer
```

### 2. Compile the Client
The client relies on Raylib to draw the game.
```bash
g++ -Wall -std=c++20 \
    Game/src/main.cpp \
    -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 \
    -o PongClient
```

---

## How to Play (Locally)

To test the game entirely on your own machine:

1. Open **Terminal 1** and start the server:
   ```bash
   ./PongServer
   ```
2. Open **Terminal 2** and start Player 1:
   ```bash
   ./PongClient
   ```
   *Type `1` to Create a Room. Note the 4-character code.*
3. Open **Terminal 3** and start Player 2:
   ```bash
   ./PongClient
   ```
   *Type `2` to Join the Room, and enter the code.*

**Controls:**
- **W** - Move Paddle Up
- **S** - Move Paddle Down
- First to 5 points wins!

---

##  Playing Over the Internet (Remote Play) [WORKING ON IT]

To play with friends on other networks without hosting a dedicated cloud server, you can use **ngrok** to tunnel your local server to the web.

1. Ensure `Game/src/main.cpp` has your Ngrok URL and port assigned to `serverAddress` and `serverPort` rather than `127.0.0.1`.
2. Start the server locally: `./PongServer`
3. Run ngrok to expose your server port: `ngrok tcp 8080` (or whatever your server is bound to).
4. Send the compiled `PongClient` executable to your friend, making sure it points to the ngrok URL you just generated!
