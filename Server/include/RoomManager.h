#pragma once // Good practice to prevent double-inclusion
#include <string.h>
#include <string>
#include <unordered_map>

#include "../../Shared/include/NetworkPackets.h"
#include "../../Game/include/GameSessions.h" 

struct playerSession {
    int socket;
    int playerID;
};

struct Room {
    int player1Socket = -1;
    std::string player1Name = "";
    int player2Socket = -1;
    std::string player2Name = "";
    GameState state; //gamestate is an attribute to NetworkPackets.h
    GameSession session;//this is an attribute to GameSessinos.h
    bool isGame = false; // Note: We will use this to check if the game started or not
};

class RoomManager {
    private:
    std::string generateCode();
    
    public:
        std::unordered_map<std::string, Room> active_rooms;
        RoomManager();
        ~RoomManager();

        std::string createRoom(int player_socket);
        bool joinRoom(std::string& code, int player_socket);
        void initialise(Room& n_r);

        Room* getRoomBySocket(int clientSocket);
        void handlePlayerInput(int clientSocket, int playerID, int action);
};