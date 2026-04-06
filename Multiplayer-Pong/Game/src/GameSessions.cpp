#include <iostream>
#include <time.h>
#include "../include/GameSessions.h"
// #include <cstdlib>
#include <cmath>

GameSession::GameSession(){
    resetGame();
    //X is Width, Y is Height
    state.ballX = SCREEN_WIDTH / 2;
    state.ballY = SCREEN_HEIGHT / 2;
    state.player1Pos = (SCREEN_HEIGHT / 2) - (PADDLE_LENGTH / 2);
    state.player2Pos = (SCREEN_HEIGHT / 2) - (PADDLE_LENGTH / 2);
    
    p1Dir = 0;
    p2Dir = 0;
}

GameSession::~GameSession(){}

void GameSession::handleInput(int playerID, int action) {
    if (playerID == 1) {
        if (action == 'w') p1Dir = -1;
        else if (action == 's') p1Dir = 1;
        else p1Dir = 0;
    } 
    else if (playerID == 2) {
        if (action == 'w') p2Dir = -1;
        else if (action == 's') p2Dir = 1;
        else p2Dir = 0;
    }
}

void GameSession::update() {
    if (state.p1_score >= 5) state.winner = 1;
    if (state.p2_score >= 5) state.winner = 2;

    if(state.winner != 0){
        return; //stop is already someone has won the match and thus stop the session
    }
    if (state.countdown > 0) {
        state.countdown--;
    } else {
        frameCounter ++;
        if (frameCounter % 120 == 0) {
            TIME_DIAL++; 
            std::cout << "Speed is now[level up]: " << TIME_DIAL << std::endl;
        }
    }

    // Move Player 1 with bounds checking
    if (p1Dir == -1 && state.player1Pos - PADDLE_VEL >= 0) 
        state.player1Pos -= PADDLE_VEL;
    if (p1Dir == 1 && state.player1Pos + PADDLE_LENGTH + PADDLE_VEL <= SCREEN_HEIGHT) 
        state.player1Pos += PADDLE_VEL;

    // Move Player 2 with bounds checking
    if (p2Dir == -1 && state.player2Pos - PADDLE_VEL >= 0) {
        state.player2Pos -= PADDLE_VEL;
    }
    if (p2Dir == 1 && state.player2Pos + PADDLE_LENGTH + PADDLE_VEL <= SCREEN_HEIGHT) {
        state.player2Pos += PADDLE_VEL;
    }

    if (state.countdown == 0) {
        ballUpdate();
    }
}

void GameSession::ballUpdate(){
    state.ballX += (BALL_VEL_X + (BALL_VEL_X / std::abs(BALL_VEL_X)) * (int)(TIME_DIAL/2));
    state.ballY += (BALL_VEL_Y + (BALL_VEL_Y / std::abs(BALL_VEL_Y)) * (int)(TIME_DIAL/2));

    // Ceiling/Floor Bounces
    if (state.ballY <= 0 || state.ballY >= SCREEN_HEIGHT) {
        BALL_VEL_Y *= -1;
        if(state.ballY <= 0) state.ballY = 1;
        if(state.ballY >= SCREEN_HEIGHT) state.ballY = SCREEN_HEIGHT - 1;
    }

    // LEFT PADDLE COLLISION (Player 1)
    if (state.ballX <= 30) {
        if (state.ballY >= state.player1Pos && state.ballY <= (state.player1Pos + PADDLE_LENGTH)) {
            BALL_VEL_X *= -1;
            state.ballX = 31; // Nudge  
            
            // Momentum Friction: Add momentumFactor of paddle speed to ball Y
            BALL_VEL_Y += (p1Dir * PADDLE_VEL * M_FACT);
        }
    }

    // RIGHT PADDLE COLLISION (Player 2)
    if (state.ballX >= SCREEN_WIDTH - 30) {
        if (state.ballY >= state.player2Pos && state.ballY <= (state.player2Pos + PADDLE_LENGTH)) {
            BALL_VEL_X *= -1;
            state.ballX = SCREEN_WIDTH - 31; // Nudge
            
            // Momentum Friction
            BALL_VEL_Y += (p2Dir * PADDLE_VEL * M_FACT);
        }
    }

    // Clamp Y velocity so the ball doesn't go insane after many hits
    // Max = 3x base speed (15). Keeps momentum mechanic fun but playable.
    if (BALL_VEL_Y > 15) BALL_VEL_Y = 15;
    if (BALL_VEL_Y < -15) BALL_VEL_Y = -15;

    // SCORING / RESET
    if (state.ballX < 0 || state.ballX > SCREEN_WIDTH) {
        if(state.ballX <= 0){
            //player 2 has scored
            state.p2_score++;
            resetBall();
        }else{
            state.p1_score++;
            resetBall();
        }
        
    }
}

void GameSession::resetBall(){
    state.countdown = 180; // 3 seconds at 60 FPS
    TIME_DIAL = 0;
    frameCounter = 1;  // Also reset the frame counter for the speed ramp

    state.ballX = SCREEN_WIDTH / 2;
    state.ballY = SCREEN_HEIGHT / 2;

    // Reset velocities to base speed, keeping X direction (serve to loser)
    BALL_VEL_X = (BALL_VEL_X > 0) ? 5 : -5;
    BALL_VEL_Y = (BALL_VEL_Y > 0) ? 5 : -5;
}

void GameSession::resetGame() {
    state.p1_score = 0;
    state.p2_score = 0;
    state.winner = 0;
    state.player1Pos = 250; // Or whatever your default is
    state.player2Pos = 250;
    resetBall();
}