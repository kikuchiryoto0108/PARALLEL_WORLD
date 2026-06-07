#pragma once

#include "common.h"

float fPlayerXMAX = 12.0f;
float fPlayerX = 11.0f;
float fPlayerY = 1.0f;
float fPlayerZ = nMapDepth - 5.0f;
float fPlayerA = 0.0f;
float fFOV = 3.14159f / 2.0f;
float fDepth = 16.0f;
float fSpeed = 12.0f;

bool is2DMode = true;
bool isJumping = false;
bool isFalling = false;
float jumpSpeed = 15.0f;
float gravity = 20.0f;
float jumpVelocity = 0.0f;


// ゴール処理用のフラグ
bool isGoalReached = false;
bool isMovingDown = false;
bool isMovingRight = false;
bool isPlayerMove = false;

// 現在のステージを保持するフラグ
bool isStage1 = true;

void InitPlayer();

// *****************************************************************************
// マリオのサイズは、始点からX+4,Y+0,Z-2　(X5,Y1,Z3)の大きさ
// *****************************************************************************

void InitPlayer() {
    // 初期化
    fPlayerX = 13.0f;
    fPlayerY = 1.0f;
    fPlayerZ = nMapDepth - 6.0f;
    fPlayerA = 0.0f;
    jumpVelocity = 0.0f;
    is2DMode = true;
    isJumping = false;
    isFalling = false;
    gravity = 20.0f;

    // ゴール処理用のフラグ
    isGoalReached = false;
    isMovingDown = false;
    isMovingRight = false;
    isPlayerMove = false;

    // スコア等
    coinCount = 0;
    score = 0;
    hotStar = 0;
    startTime = std::chrono::steady_clock::now(); // 時間の初期化

    isStage1 = true;
}
