/*This holds all the rules and laws for each gaming sessions*/

#pragma once
#include <string>
#include <cstring>
#include "../../Shared/include/NetworkPackets.h"
class GameSession{
    private:
        
        //cant use windows.h to get screen matrix due to issues with 
        //game sync and screen represenation
        int SCREEN_WIDTH = 800; 
        int SCREEN_HEIGHT = 600;
        int PADDLE_VEL = 10;
        int PADDLE_LENGTH = 100;

        int p1Dir = 0; //-1 for up 1 for down 0 for constnat
        int p2Dir = 0; //-1 for up 1 for down 0 for constnat
        static constexpr float M_FACT = 0.7;
        int frameCounter= 1;
        //there will be incrementals
        int BALL_VEL_X = 5;
        int BALL_VEL_Y = 5;

        int TIME_DIAL = 1; // calculating increase values through this time dial

        void ballUpdate();

    public:
        GameState state;
        GameSession();
        ~GameSession();

        void update();//for each Frame
        void handleInput(int playerID, int action);
        GameState getState() {return state;}
        void resetBall();
        void resetGame();
};