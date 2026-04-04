/*this is nothing but a calulating env*/
#include "../include/RoomManager.h"
#include <random>
#include <iostream>
#include <map>

RoomManager::RoomManager(){};
RoomManager::~RoomManager(){};

//generation of the room Code
std::string RoomManager::generateCode(){
    const char hexChar[] = "1234567890ABCDEF";/*hexadecimal approach*/

    /* initialising the random instances */
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15); /*hexadecimal appraoch*/

    std::string code;
    for(int i = 0; i < 4;i++){
        code += hexChar[dis(gen)]; /*add random letters to code */
    }

    return code;
}


std::string RoomManager::createRoom(int player_socket) {
    std::string newCode= generateCode();

    while(active_rooms.contains(newCode)){
        newCode = generateCode();
    }

    Room rm;
    rm.player1Socket = player_socket;

    active_rooms[newCode]= rm;
    std::cout<<"New Room created by -> "<< player_socket<< " with ID "<<newCode<<"\n";

    return newCode;
}

void RoomManager::initialise(Room& n_r){
    n_r.session.state.player1Pos = 250;
    n_r.session.state.player2Pos = 250;
    n_r.session.state.ballX = 400;
    n_r.session.state.ballY = 300;
    
    std::cout << "Physics Engine Initialized for Room." << std::endl;
}

bool RoomManager::joinRoom(std::string& code, int player_socket){
    
    if(!active_rooms.contains(code)){
        std::cout<<"Player "<<player_socket<<" has entered wrong code \n";
        return false;
    }

    Room& rm = active_rooms[code];

    if (rm.player2Socket != -1) {
        std::cout << "Room " << code << " is already full!\n";
        return false;
    }

    rm.player2Socket= player_socket;
    rm.isGame = true;
    initialise(rm);
    
    std::cout << "Player " << player_socket << " successfully joined room " << code << "\n";

    return true;
}

Room* RoomManager::getRoomBySocket(int clientSocket) {
    // Search through your 'active_rooms' map
    for (auto& pair : active_rooms) {
        if (pair.second.player1Socket == clientSocket || 
            pair.second.player2Socket == clientSocket) {
            return &pair.second; // Found the room whcich is active
        }
    }
    return nullptr; // Player is not in any room
}

void RoomManager::handlePlayerInput(int clientSocket, int playerID, int action) {
    // Find which room this socket belongs to
    Room* room = getRoomBySocket(clientSocket);

    if (room != nullptr) {
        // Only accept W/S input if the room is full and the game started
        if (room->isGame) { 
            // Hand the input to the physics engine!
            room->session.handleInput(playerID, action);
        }
    } else {
        std::cerr << "Input received from socket " << clientSocket << " but they aren't in a room!\n";
    }
}