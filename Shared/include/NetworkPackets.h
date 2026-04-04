#pragma once
#include <string>

enum PacketType {
    PACKET_HANDSHAKE = 1,
    PACKET_INPUT = 2,
    PACKET_GAME_STATE = 3
};

struct GameState{
    PacketType type = PACKET_GAME_STATE;
    int player1Pos = -1;
    int player2Pos = -1;
    int ballX = -1;
    int ballY = -1;
};

struct clientHandshake{
    PacketType type = PACKET_HANDSHAKE;
    int action; //create room or join room
    char roomCode[5];
};

struct inputPacket{
    PacketType type = PACKET_INPUT;
    int playerID;
    int playerAction; /*'w' is move up and 's' is move down*/
};
/*this is the bandwith struct and integration as used for the transfer of data over TCP*/