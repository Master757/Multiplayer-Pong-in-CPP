#pragma once

#include <vector>
#include <map>
#include <poll.h>
#include <netinet/in.h>

class RoomManager; //declaring the class to make a poiter to it

class TCPServer 
{
    private:
        int serverSocket;
        int port;

        std::vector<pollfd> pollfds;
        RoomManager* roomManager;   
        std::map<int, std::vector<char>> clientBuffers;

        /*---functions---*/

        void acceptNewConnections();
        void handleClientData(int p_indx);
        void disconnectClient(int p_indx);

    public:
        TCPServer(int pt, RoomManager* rm);
        ~TCPServer();
        bool start(); //to start the transmissions
        void processNetwork();
        void send_data(int clientSocket, const void* data, size_t datasize);

};
