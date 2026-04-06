#pragma once

#include<iostream>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../../Shared/include/NetworkPackets.h"

class GameClient{
    private:
        int playerSocket;
        std::string roomID;
        bool connected;
        int playerID;//1 or 2

    public:
        GameClient();
        ~GameClient();

        //networking core
        bool connectToServer(const char* ip, int port);
        void disconnect();

        //Requests
        bool reqCreateRoom();
        bool const reqJoinRoom(const std::string& roomID);

        //Game Comm
        void sendPlayerInput(float newPos);
        bool recvGameState(GameState& outState);

        /*These are getter functions which we need later on*/
        bool isConnected() const {return connected;};
        std::string getRoomCode() const {return roomID;};
        int getPlayerID() const {return playerID;};
        int getPlayerSocket() const {return playerSocket;};
};