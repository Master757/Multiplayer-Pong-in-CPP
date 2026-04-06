#include "../include/gameClient.h"
#include <iostream>
#include <cstring>

GameClient::GameClient() : playerSocket(-1), connected(false), playerID(0) {
}

GameClient::~GameClient() {
    disconnect();
}

void GameClient::disconnect(){
    if(connected){
        close(playerSocket);
        playerSocket = -1;
        playerID = -1;
        connected = false;
        std::cout << "Disconnected [in gameClient.cpp, function Line 15]\n";
    }
}

bool GameClient::connectToServer(const char* ip, int port){
    playerSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if(playerSocket < 0){
        std::cerr << "There has been an Error in socket creation [gameClient.cpp, Line 24]\n";
        return false;
    }

    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_port = htons(port); 
    addr.sin6_family = AF_INET6;

    if (inet_pton(AF_INET6, ip, &addr.sin6_addr) <= 0) {
        std::cerr << "Error: Invalid IP address [gameClient.cpp, Line 30-33]\n";
        return false;
    }

    int connectCheck = connect(playerSocket, (struct sockaddr*)&addr, sizeof(addr));
    if(connectCheck < 0){
        std::cerr << "There has been an issue with Connection [gameClient.cpp, Line 40]\n";
        return false;
    } 

    connected = true;
    std::cout << "Connection is Successful [from gameClient.cpp]\n";
    return true;
}

bool GameClient::reqCreateRoom(){ // Fixed: Capital G in GameClient
    if(playerSocket < 0){
        std::cerr << "There was error in gameClient.cpp, Line52\n";
        return false;
    }

    clientHandshake gameClientPacket = {};  // Zero-initialize all fields
    gameClientPacket.type = PACKET_HANDSHAKE;  // Set type AFTER zeroing!
    gameClientPacket.action = 1;
    
    if (send(playerSocket, &gameClientPacket, sizeof(gameClientPacket), 0) < 0) {
        std::cerr << "Failed to send Create Room request.\n";
        return false;
    }

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    int recv_bytes = recv(playerSocket, buffer, sizeof(buffer) - 1, 0);

    if(recv_bytes == 0){
        std::cout << "no connection established as stated in gameClient.cpp,Line 70\n";
        return false;
    } else if(recv_bytes < 0){
        std::cerr << "There occurred an error in receiving bytes [gameClient.cpp, Line 70]\n";
        return false; 
    } else {
        this->roomID = std::string(buffer);
        this->playerID = 1;
        return true;
    }
}

bool const GameClient::reqJoinRoom(const std::string& code){
    if(playerSocket < 0){
        std::cerr << "Cannot join, socket is invalid.\n";
        return false;
    }

    clientHandshake gameClientPacket = {};  // Zero-initialize all fields
    gameClientPacket.type = PACKET_HANDSHAKE;  // Set type AFTER zeroing!
    gameClientPacket.action = 2; // Action 2 = Join Room
    
    // Safely copy the string code into the struct's char array
    strncpy(gameClientPacket.roomCode, code.c_str(), sizeof(gameClientPacket.roomCode) - 1);

    if (send(playerSocket, &gameClientPacket, sizeof(gameClientPacket), 0) < 0) {
        std::cerr << "Failed to send Join Room request.\n";
        return false;
    }

    // We assume success if the server doesn't instantly drop our connection
    this->roomID = code;    
    this->playerID = 2; // The joiner is always Player 2
    return true;
}