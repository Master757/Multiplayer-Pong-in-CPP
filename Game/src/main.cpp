#include "raylib.h"
#include "../../Shared/include/NetworkPackets.h"
#include <iostream>
#include <string>
#include <cstring>
#include <netdb.h>  
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <exception>

int main() {
    std::cout << "--- Pong Client ---\n";
    
    // 1. SETUP NETWORK (NGROK TUNNEL)
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    
    // --- YOUR NGROK TUNNEL ---
    std::string serverAddress = "0.tcp.in.ngrok.io"; 
    int serverPort = SERVER_PORT;                       

    serv_addr.sin_port = htons(serverPort);

    // Translate the URL into an IP
    struct hostent* host = gethostbyname(serverAddress.c_str());
    if (host == nullptr) {
        std::cerr << "Could not resolve hostname!\n";
        return -1;
    }
    
    std::memcpy(&serv_addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed!\n";
        return -1;
    }

    // 2. THE HANDSHAKE (Create or Join Room)
    int choice;
    int myPlayerID = 1; 
    std::cout << "1. Create Room\n2. Join Room\nSelect: ";
    std::cin >> choice;

    clientHandshake handshake;
    handshake.type = PACKET_HANDSHAKE;
    char roomCodeBuf[6] = "NONE";
    
    if (choice == 1) {
        handshake.action = 1; // Create
        myPlayerID = 1;       // Creator is always P1
        send(sock, (char*)&handshake, sizeof(clientHandshake), 0);
        
        recv(sock, roomCodeBuf, 5, 0); // Wait for server to give us the code
        std::cout << "Room Created! Tell your friend code: " << roomCodeBuf << "\n";

    } else {
        handshake.action = 2; // Join
        myPlayerID = 2;       // Joiner is always P2
        std::cout << "Enter 4-letter Room Code: ";
        std::cin >> handshake.roomCode;
        send(sock, (char*)&handshake, sizeof(clientHandshake), 0);
        
        char responseBuf[6] = {0};
        recv(sock, responseBuf, 5, 0);
        std::cout << "Joined Room: " << responseBuf << "\n";
    }

    // Set socket to non-blocking AFTER handshake so the game doesn't freeze
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // 3. LAUNCH RAYLIB GRAPHICS
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Multiplayer Pong");
    SetTargetFPS(60);

    GameState latestState; // Holds the truth from the server

    // 4. MAIN GAME LOOP
    while (!WindowShouldClose()) {
        
        // --- STEP A: SEND INPUT ---
        inputPacket myInput;
        myInput.type = PACKET_INPUT; // The ID Tag!
        myInput.playerID = myPlayerID;
        
        if (IsKeyDown(KEY_W)) myInput.playerAction = 'w';
        else if (IsKeyDown(KEY_S)) myInput.playerAction = 's';
        else myInput.playerAction = 'n'; // None

        send(sock, (char*)&myInput, sizeof(inputPacket), 0);

        // --- STEP B: RECEIVE TRUTH (DRAIN THE QUEUE) ---
        GameState tempState;
        while (recv(sock, (char*)&tempState, sizeof(GameState), 0) == sizeof(GameState)) {
            latestState = tempState; 
        }

        // --- STEP C: DRAW ---
        BeginDrawing();
            ClearBackground(BLACK);

            // If coordinates are -1, the game hasn't started yet
            if (latestState.ballX == -1) {
                // Now it actually shows the room code on the screen for Player 1!
                DrawText(TextFormat("ROOM CODE: %s - WAITING FOR P2...", roomCodeBuf), 150, 280, 20, LIGHTGRAY);
            } else {
                // Draw Center Line
                DrawLine(screenWidth / 2, 0, screenWidth / 2, screenHeight, DARKGRAY);

                // Draw Ball
                DrawCircle(latestState.ballX, latestState.ballY, 10, RAYWHITE);

                // Draw Paddles
                DrawRectangle(10, latestState.player1Pos, 20, 100, SKYBLUE); // P1
                DrawRectangle(screenWidth - 30, latestState.player2Pos, 20, 100, RED); // P2
            }

        EndDrawing();
    }

    close(sock);
    CloseWindow();
    return 0;
}