#include "../../Game/include/GameSessions.h"
#include <iostream>
#include <csignal>
#include "../include/RoomManager.h"
#include "../include/TCPServer.h"
#include <chrono>
#include <thread>
#include <exception>

int main() {
    signal(SIGPIPE, SIG_IGN);
    std::cout << "=== Pong Multi ===" << std::endl;
    RoomManager roomManager;

    TCPServer server(8080, &roomManager);

    if (!server.start()) {
        std::cerr << "There was an error in server start in main.cpp\n";
        return 1; 
    }

    // --- THE 60 FPS HEARTBEAT LOOP ---
    while (true) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // NETWORK: Check for new players or keypresses (just ONCE per frame)
        server.processNetwork(); 

        // PHYSICS & BROADCAST: Tick all active games
        for (auto& pair : roomManager.active_rooms) {
            Room& room = pair.second;
            
            // Only run physics if Player 2 has joined
            if (room.isGame) {
                    
                room.session.update();
                // BROADCAST: Send the authoritative state to both players (once per frame)
                if (room.player1Socket != -1) {
                    server.send_data(room.player1Socket, &room.session.state, sizeof(GameState));
                }
                if (room.player2Socket != -1) {
                    server.send_data(room.player2Socket, &room.session.state, sizeof(GameState));
                }

            }
        }

        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> elapsed = frameEnd - frameStart;
        if (elapsed.count() < 16.6f) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16) - 
                std::chrono::duration_cast<std::chrono::milliseconds>(elapsed));
        }
    }

    return 0;
}