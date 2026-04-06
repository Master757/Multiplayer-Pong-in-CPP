#include <iostream>
#include "../include/TCPServer.h"
#include "../../Shared/include/NetworkPackets.h"
#include "../include/RoomManager.h"
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

TCPServer::TCPServer(int pt, RoomManager* rm) : port(pt), roomManager(rm), serverSocket(-1){
    std::cout<<"Debug: Constructor for TCP called \n";
}

TCPServer::~TCPServer(){
    for(int i = 0 ; i < pollfds.size(); i++){
        close(pollfds[i].fd);
    }

    std::cout<<"Server shut down sucessfull, all sockects closed for connection \n";
}

bool TCPServer::start(){
    serverSocket= socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if(serverSocket < 0){
        std::cerr<<"An error has occuredin TCPServer start, failed to create socket \n";
    }

    //after conection establishment
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //setting up adress structure
    sockaddr_in6 serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); //clearning adress mem before setup
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_addr = in6addr_any;
    serverAddr.sin6_port = htons(port);

    int connection_bind = bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    if(connection_bind < 0){
        std::cerr << "There was a problem in binding [TCPServer.cpp, line37]\n";
        return false;
    }

    if (listen(serverSocket, SOMAXCONN) < 0) {
        std::cerr << "Critical Error: Socket failed to listen!\n";
        return false;
    }
    
    /*
    The exact 'C' structure of pollfd

    struct pollfd {
        int   fd;         // File descriptor (the socket ID)
        short events;     // What you want the OS to watch for
        short revents;    // What actually happened (filled in by the OS)
    };
    */
    pollfd serverPollfd;
    serverPollfd.fd = serverSocket;
    serverPollfd.events= POLLIN;
    serverPollfd.revents = 0;

    pollfds.push_back(serverPollfd);

    std::cout<<"The TCP server has started sucessfully, \n";
    return true;
}

void TCPServer::processNetwork() {
    
    int pollCount = poll(pollfds.data(), pollfds.size(), 0); 
    
    if (pollCount < 0) {
        std::cerr << "Critical error: poll failed[TCPServer.cpp Line 73] \n";
        return;
    }

    for (auto i = 0 ; i < pollfds.size(); i++) {
        if (pollfds[i].revents & POLLIN) {
            if (pollfds[i].fd == serverSocket) {
                acceptNewConnections();
            } else {
                handleClientData(i); // This now routes input to RoomManager!
            }
        }
    }
}

void TCPServer::acceptNewConnections(){
    sockaddr_in6 clientAddr;
    socklen_t clientAddrLen= sizeof(clientAddr);

    int clientSocket= accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

    if(clientSocket < 0){
        std::cerr<<"There was an error in making new connection [TCPServer.cpp, Line 91]\n";
        return;
    }
    //creating a pollFd for the client
    pollfd clientPollFd;
    clientPollFd.fd = clientSocket;
    clientPollFd.events = POLLIN; // Tell OS: "Watch this player for data"
    clientPollFd.revents = 0;

    pollfds.push_back(clientPollFd);

    std::cout<<"A new player has joined the server\n";
}


void TCPServer::disconnectClient(int p_indx){
    if(pollfds[p_indx].fd < 0){
        std::cerr<<"Connection already severed\n";
        return;  // Don't try to close an already-closed socket!
    }

    int socketToClose = pollfds[p_indx].fd;

    // IMPORTANT: Tell RoomManager FIRST (before we close the fd)
    // so it can find the room by socket number and clean it up.
    roomManager->handleDisconnect(socketToClose);

    close(socketToClose);
    pollfds.erase(pollfds.begin() + p_indx);
    clientBuffers.erase(socketToClose);
}

void TCPServer::handleClientData(int p_indx){
    int clientSocket= pollfds[p_indx].fd;

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));

    int bytes_recv = recv(clientSocket, buffer, sizeof(buffer), 0);
    if(bytes_recv < 0){
        std::cerr<<"There was an error in the bytes recieved [TCPServer.cpp, Line: 124]\n";
        disconnectClient(p_indx);
        return;
    }else if (bytes_recv == 0){
        std::cout<<"The connectoin was closed by player, "<<clientSocket<<" , deleting connection...\n";
        disconnectClient(p_indx);
        return;
    }

    // Append new bytes to this client's persistent buffer
    std::vector<char>& cbuf = clientBuffers[clientSocket];
    cbuf.insert(cbuf.end(), buffer, buffer + bytes_recv);

    /*
     * HOW PACKET ROUTING WORKS:
     * We loop through the persistent buffer and process ALL complete packets.
     * Partial packets stay in the buffer until the next chunk of bytes arrives.
     */
    while (cbuf.size() > 0) {
        if (cbuf.size() < sizeof(PacketType)) {
            // Not enough bytes for even the header; wait for more data.
            break; 
        }

        PacketType type = *(PacketType*)(cbuf.data());

        if (type == PACKET_HANDSHAKE && cbuf.size() >= sizeof(clientHandshake)) {
            clientHandshake* handshake = (clientHandshake*)(cbuf.data());

            if(handshake->action == 1){
                std::string newRoomCode = roomManager->createRoom(clientSocket);
                std::cout<<"NEW ROOM CREATED, room code: "<<newRoomCode<<std::endl;
                send_data(clientSocket, newRoomCode.c_str(), newRoomCode.length());
            }else if(handshake->action == 2){
                std::string roomID = handshake->roomCode;
                bool success_joining= roomManager->joinRoom(roomID, clientSocket);
                if(success_joining){
                    std::cout<<"Player has joined a room with ID: "<<roomID<<std::endl;
                    send_data(clientSocket, roomID.c_str(), roomID.length());
                }else{
                    std::cout<<"There was some problem while joining the game\n";
                }
            }
            // Remove the parsed packet from the front of the buffer
            cbuf.erase(cbuf.begin(), cbuf.begin() + sizeof(clientHandshake));

        } else if (type == PACKET_INPUT && cbuf.size() >= sizeof(inputPacket)) {
            inputPacket* in_state = (inputPacket*)(cbuf.data());
            roomManager->handlePlayerInput(clientSocket, in_state->playerID, in_state->playerAction);
            
            // Remove the parsed packet from the front of the buffer
            cbuf.erase(cbuf.begin(), cbuf.begin() + sizeof(inputPacket));

        } else {
            // We have a type, but not enough bytes for the FULL struct yet.
            // Just break and wait for the rest of the stream to arrive.
            break;
        }
    }

}

void TCPServer::send_data(int clientSocket, const void* data, size_t datasize){
    if (clientSocket <= 0) return;
    int bytesSent = send(clientSocket, data, datasize, 0);
    if(bytesSent < 0){
        std::cerr<<"There was an error in sending msgs [TCPServer.cpp, Line 169]\n";
    }
}

