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
    std::cout << "--- Pong Client ---" << std::endl;
    
    // 1. SETUP NETWORK (NGROK TUNNEL)
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    
    // --- LOCAL TESTING ---
    std::string serverAddress = "127.0.0.1"; 
    int serverPort = 8080;                       

    serv_addr.sin_port = htons(serverPort);

    // Translate the URL into an IP
    std::cout << "Resolving hostname: " << serverAddress << "..." << std::endl;
    struct hostent* host = gethostbyname(serverAddress.c_str());
    if (host == nullptr) {
        std::cerr << "Could not resolve hostname!" << std::endl;
        return -1;
    }
    
    std::memcpy(&serv_addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

    std::cout << "Connecting to server at " << serverAddress << ":" << serverPort << "..." << std::endl;
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed! Is the server or ngrok running?" << std::endl;
        return -1;
    }
    std::cout << "Connected successfully!" << std::endl;

    // 2. THE HANDSHAKE (Create or Join Room)
    int choice;
    int myPlayerID = 1; 
    std::cout << "\n1. Create Room\n2. Join Room\nSelect: ";
    std::cout.flush();
    std::cin >> choice;

    clientHandshake handshake;
    handshake.type = PACKET_HANDSHAKE;
    char roomCodeBuf[6] = "NONE";
    
    if (choice == 1) {
        handshake.action = 1; // Create
        myPlayerID = 1;       // Creator is always P1
        send(sock, (char*)&handshake, sizeof(clientHandshake), 0);
        
        recv(sock, roomCodeBuf, 4, 0); // MUST be 4 bytes! Server only sends 4.
        roomCodeBuf[4] = '\0'; // Manually null terminate
        std::cout << "Room Created! Tell your friend code: " << roomCodeBuf << "\n";

    } else {
        handshake.action = 2; // Join
        myPlayerID = 2;       // Joiner is always P2
        std::cout << "Enter 4-letter Room Code: ";

        // BUG 8 FIX: Read into a std::string first (safe, no overflow),
        // then copy at most 4 chars into the fixed-size char[5] buffer.
        std::string codeInput;
        std::cin >> codeInput;
        strncpy(handshake.roomCode, codeInput.c_str(), sizeof(handshake.roomCode) - 1);
        handshake.roomCode[sizeof(handshake.roomCode) - 1] = '\0'; // Ensure null-terminated

        send(sock, (char*)&handshake, sizeof(clientHandshake), 0);
        
        char responseBuf[6] = {0};
        recv(sock, responseBuf, 4, 0); // MUST be 4 bytes! Server only sends 4.
        responseBuf[4] = '\0';
        std::cout << "Joined Room: " << responseBuf << "\n";

        // BUG 7 FIX: Copy the room code so the waiting screen shows the real code,
        // not "NONE"
        strncpy(roomCodeBuf, responseBuf, 5);
    }

    // Set socket to non-blocking AFTER handshake so the game doesn't freeze
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // 3. LAUNCH RAYLIB GRAPHICS
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Multiplayer Pong");
    SetTargetFPS(60);

    GameState latestState = {};

    // BUG 9 FIX: TCP Framing Buffer
    // TCP is a STREAM — recv() can return partial data or merged data.
    // This buffer accumulates bytes until we have a complete GameState.
    char recvBuffer[sizeof(GameState) * 4];  // Room for up to 4 states
    int bufferFill = 0;                       // How many bytes are in the buffer

    // 4. MAIN GAME LOOP
    while (!WindowShouldClose()) {
        
        // --- STEP A: SEND INPUT ---
        // BUG 5 FIX: Only send input when the game is actually running.
        // No point flooding the server while waiting for P2 or after game over.
        if (latestState.ballX != -1 && latestState.winner == 0) {
            inputPacket myInput;
            myInput.type = PACKET_INPUT; // The ID Tag!
            myInput.playerID = myPlayerID;
            
            if (IsKeyDown(KEY_W)) myInput.playerAction = 'w';
            else if (IsKeyDown(KEY_S)) myInput.playerAction = 's';
            else myInput.playerAction = 'n'; // None

            send(sock, (char*)&myInput, sizeof(inputPacket), 0);
        }

        // --- STEP B: RECEIVE TRUTH (with proper TCP framing) ---
        // Read whatever bytes are available into our buffer
        int bytesRead = recv(sock, recvBuffer + bufferFill, 
                            sizeof(recvBuffer) - bufferFill, 0);
        if (bytesRead > 0) {
            bufferFill += bytesRead;
        }

        // Parse as many complete GameState structs as we have
        while (bufferFill >= (int)sizeof(GameState)) {
            // Copy a full GameState from the front of the buffer
            std::memcpy(&latestState, recvBuffer, sizeof(GameState));

            // Shift remaining bytes to the front (remove the parsed state)
            bufferFill -= sizeof(GameState);
            if (bufferFill > 0) {
                std::memmove(recvBuffer, recvBuffer + sizeof(GameState), bufferFill);
            }
        }

        // --- STEP C: DRAW ---
        BeginDrawing();
            ClearBackground(BLACK);

            if (latestState.winner != 0) {
                // GAME OVER SCREEN
                if (latestState.winner == myPlayerID) {
                    DrawText("VICTORY!", 280, 200, 50, GREEN);
                } else {
                    DrawText("DEFEAT...", 280, 200, 50, RED);
                }
                DrawText(TextFormat("FINAL SCORE: %d - %d", latestState.p1_score, latestState.p2_score   ), 260, 300, 20, RAYWHITE);
                DrawText("CLOSE WINDOW TO QUIT", 280, 400, 20, GRAY);
            }
            else if (latestState.ballX == -1) {
                // WAITING ROOM
                DrawText(TextFormat("ROOM CODE: %s - WAITING FOR P2...", roomCodeBuf), 150, 280, 20, LIGHTGRAY);
            } 
            else {
                // ACTIVE GAME
                // Draw Scores
                DrawText(TextFormat("%d", latestState.p1_score), 200, 50, 40, SKYBLUE);
                DrawText(TextFormat("%d", latestState.p2_score), 560, 50, 40, RED);

                // Draw Center Line, Ball, and Paddles
                DrawLine(screenWidth / 2, 0, screenWidth / 2, screenHeight, DARKGRAY);
                DrawCircle(latestState.ballX, latestState.ballY, 10, RAYWHITE);
                DrawRectangle(10, latestState.player1Pos, 20, 100, SKYBLUE); 
                DrawRectangle(screenWidth - 30, latestState.player2Pos, 20, 100, RED); 

                // Draw Countdown Timer if active
                if (latestState.countdown > 0) {
                    int secondsLeft = (latestState.countdown / 60) + 1;
                    DrawText(TextFormat("GET READY: %d", secondsLeft), 280, 200, 40, YELLOW);
                }
            }

        EndDrawing();
    }

    close(sock);
    CloseWindow();
    return 0;
}