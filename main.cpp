#include "common.h"
#include "player.h"
#include "title.h"
#include "sound.h"

#define DEBUG_TIMER (500)    // デバッグ表示の間隔 500ms
#define FRICTION 0.85        // 摩擦係数

// シーンの定義
enum Scene {
    TitleScene,
    SelectScene,
    Draw0Scene,
    Draw1Scene,
    Draw2Scene,
    TutorialScene,
    Stage1_1Scene,
    Stage1_2Scene,
    ResultScene,
    GameOverScene,
};

// *****************************************************************************
// グローバル変数
// *****************************************************************************
float fElapsedTime;

int g_fpsCounter;                // FPSカウンタ
float fCameraX = 0.0f;
float fCameraY = 0.0f;
float fCameraZ = 0.0f;
float fCameraAngleH = 0.0f;
float fCameraAngleV = fFOV / 1111.0f; // 斜め下方向に向ける

Scene currentScene = TitleScene; // 現在のシーン
bool isGameOver = false;         // ゲームオーバーフラグ

// バーナーの状態
bool isBurnerActive = true;
std::chrono::time_point<std::chrono::steady_clock> lastBurnerToggle;

std::chrono::time_point<std::chrono::steady_clock> lastPipeEntry;  // 土管に入った最後の時間
bool isPipeCoolingDown = false;  // 土管のクールダウン中かどうか

// プレイヤーの当たり判定サイズ
const int playerWidth = 5;
const int playerHeight = 3;

static int cachedTime = maxTimeLimit;

int temp = 0;
DWORD dwBytesWritten = 0;

bool isSwitchStage = false; // 土管遷移

static char BGM_soundfile[] = "Sounds\\BGM.mp3";

bool firstCheak = true;

bool bgmCheak = true;

// *****************************************************************************
// プロトタイプ宣言
// *****************************************************************************
void InitializeStageFromString();
void HideConsoleCursor(HANDLE hConsole);
void DrawPlayer(wchar_t* screen, WORD* screenColor);
void Draw2D(wchar_t* screen, WORD* screenColor);
void Draw3D(wchar_t* screen, WORD* screenColor);
void PlayerMovement(wchar_t* screen, WORD* screenColor, HANDLE hScreen);
void HandlePhysics(float fElapsedTime);
void UpdateCameraPosition();
void ToggleBurner();
void DrawScoreCoinAndTime(wchar_t* screen, WORD* screenColor);
bool CheckGoalCondition();
void CheckTimeLimit();
void ProcessGoal(wchar_t* screen, WORD* screenColor, HANDLE hScreen);
void Update2DCameraPosition();
void Update3DCameraPosition();
void SwitchStage(wchar_t* screen, WORD* screenColor, HANDLE hScreen);
void DrawResultScene(wchar_t* screen, WORD* screenColor);
void DrawGameOverScene(wchar_t* screen, WORD* screenColor);
void DrawSelectScene(wchar_t* screen, WORD* screenColor);
void Draw0(wchar_t* screen, WORD* screenColor);
void Draw1(wchar_t* screen, WORD* screenColor);
void Draw2(wchar_t* screen, WORD* screenColor);
int inport(int port);

// *****************************************************************************
// 関数
// *****************************************************************************
int inport(int port)
{
    DWORD	dwEvent;
    HANDLE	h;

    /* ゲームパッド入力 */
    if ((port & 0xfe00) == 0x0200) {
        int id = (port & 0x01f0) >> 4;
        int func = port & 0x0f;
        JOYINFOEX ji;
        ji.dwSize = sizeof(JOYINFOEX);
        ji.dwFlags = JOY_RETURNALL;

        if (joyGetPosEx(id, &ji) != JOYERR_NOERROR)
            return 32767;

        switch (func) {
        case 0:
            return ji.dwXpos;
        case 1:
            return ji.dwYpos;
        case 2:
            return ji.dwZpos;
        case 3:
            return ji.dwButtons;
        case 4:
            return ji.dwRpos;
        case 5:
            return ji.dwUpos;
        case 6:
            return ji.dwVpos;
        case 7:
            return ji.dwButtonNumber;
        case 8:
            return ji.dwPOV;
        case 9:
            return ji.dwReserved1;
        case 10:
            return ji.dwReserved2;
        default:
            break;
        }
        return 0;
    }
}

// マップをステージに3次元配列としてぶち込む
void InitializeStageFromString() {
    std::string* stageMap1;
    std::string* stageMap2;

    switch (currentScene) {
    case TutorialScene:
        stageMap1 = stageMapTutorial1;
        stageMap2 = stageMapTutorial2;
        break;
    case Stage1_1Scene:
        stageMap1 = stageMap1_1_1;
        stageMap2 = stageMap1_1_2;
        break;
    case Stage1_2Scene:
        stageMap1 = stageMap1_2_1;
        stageMap2 = stageMap1_2_2;
        break;
    default:
        stageMap1 = stageMapTutorial1;
        stageMap2 = stageMapTutorial2;
        break;
    }
    for (int z = 0; z < nMapDepth; z++) {
        for (int y = 0; y < nMapHeight; y++) {
            for (int x = 0; x < nMapWidth; x++) {
                stage1[z][y][x] = stageMap1[z][y * nMapWidth + x];
            }
        }
    }
    for (int z = 0; z < nMapDepth; z++) {
        for (int y = 0; y < nMapHeight; y++) {
            for (int x = 0; x < nMapWidth; x++) {
                stage2[z][y][x] = stageMap2[z][y * nMapWidth + x];
            }
        }
    }
}

// コンソールカーソルを非表示にする関数
void HideConsoleCursor(HANDLE hConsole) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = false; // カーソルを非表示にする
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

// 2Dカメラ位置の更新
void Update2DCameraPosition() {
    if (fPlayerX >= 40 && (nMapWidth - fPlayerX) >= 40) {
        fCameraX = fPlayerX - 40;
    } else if (fPlayerX < 40) {
        fCameraX = 0;
    } else {
        fCameraX = nMapWidth - nScreenWidth;
    }
}

// 3Dカメラ位置の更新
void Update3DCameraPosition() {
}

// UpdateCameraPosition関数の修正
void UpdateCameraPosition() {
    if (is2DMode) {
        Update2DCameraPosition();
    } else {
        Update3DCameraPosition();
    }
}

int nPlayerPosition = 0; // 0: Tutorial, 1: Stage1_1, 2: Stage1_2
// プレイヤーの移動方向
enum MoveDirection {
    None,
    Left,
    Right
};
MoveDirection moveDirection = None;
// ステージセレクトシーンの描画
void DrawSelectScene(wchar_t* screen, WORD* screenColor) {
    if (firstCheak) {
        PlayBGM(L"Sounds/BGM.wav", 50);
        fPlayerX = 14;
        fPlayerZ = 13;
        fPlayerY = 1;
        fCameraX = 0;
        firstCheak = false;
        switch (nPlayerPosition) {
        case 0:
            fPlayerX = 14;
            break;
        case 1:
            fPlayerX = 38;
            break;
        case 2:
            fPlayerX = 62;
            break;
        }
    }
    std::wstring stage1 = L"Tutorial";
    std::wstring stage2 = L"Normal";
    std::wstring stage3 = L"Hard";
    std::string stageMap[nMapDepth] = {
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "..............YYYYY...................YYYYY...................YYYYY.............",
        ".............YBBBBBY.................YBBBBBY.................YBBBBBY............",
        "............YBBBBBBBYMMMMMMMMMMMMMMMYBBBBBBBYMMMMMMMMMMMMMMMYBBBBBBBY...........",
        "............YBBBBBBBYMMMMMMMMMMMMMMMYBBBBBBBYMMMMMMMMMMMMMMMYBBBBBBBY...........",
        ".............YBBBBBY.................YBBBBBY.................YBBBBBY............",
        "..............YYYYY...................YYYYY...................YYYYY.............",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
        "................................................................................",
    };
    int stage1Start = 17 - stage1.size() / 2;
    int stage2Start = 41 - stage2.size() / 2;
    int stage3Start = 64 - stage3.size() / 2;

    for (int y = 0; y < nScreenHeight; y++) {
        for (int x = 0; x < nScreenWidth; x++) {
            switch (stageMap[y][x]) {
            case '.':
                screenColor[y * nScreenWidth + x] = FG_GREEN | BG_GREEN;
                break;
            case 'B':
                screenColor[y * nScreenWidth + x] = FG_BLUE | BG_BLUE;
                break;
            case 'Y':
                screenColor[y * nScreenWidth + x] = (0 | FOREGROUND_INTENSITY) | (0 | BACKGROUND_INTENSITY);
                break;
            case 'M':
                screenColor[y * nScreenWidth + x] = FG_DARK_YELLOW | BG_DARK_YELLOW;
                break;
            }
        }
    }
    // 描画
    for (size_t i = 0; i < stage1.size(); i++) {
        screen[nScreenWidth * ((nScreenHeight / 2) - 5) + stage1Start + i] = stage1[i];
        screenColor[nScreenWidth * ((nScreenHeight / 2) - 5) + stage1Start + i] = FG_WHITE | BG_GREEN;
    }

    for (size_t i = 0; i < stage2.size(); i++) {
        screen[nScreenWidth * ((nScreenHeight / 2) - 5) + stage2Start + i] = stage2[i];
        screenColor[nScreenWidth * ((nScreenHeight / 2) - 5) + stage2Start + i] = FG_WHITE | BG_GREEN;
    }

    for (size_t i = 0; i < stage3.size(); i++) {
        screen[nScreenWidth * ((nScreenHeight / 2) - 5) + stage3Start + i] = stage3[i];
        screenColor[nScreenWidth * ((nScreenHeight / 2) - 5) + stage3Start + i] = FG_WHITE | BG_GREEN;
    }

    // プレイヤーの移動
    if (moveDirection == None) {
        if ((GetAsyncKeyState((unsigned short)'A') & 0x8000 || inport(0x0200) < 31000 || inport(0x0210) < 31000 || inport(0x0220) < 31000 || inport(0x0230) < 31000) && nPlayerPosition > 0) {
            moveDirection = Left;
        }

        if ((GetAsyncKeyState((unsigned short)'D') & 0x8000 || inport(0x0200) > 34000 || inport(0x0210) > 34000 || inport(0x0220) > 34000 || inport(0x0230) > 34000) && nPlayerPosition < 2) {
            moveDirection = Right;
        }
    }

    // プレイヤーの自動移動
    if (moveDirection == Left) {
        fPlayerX -= 1.0f;
        if ((nPlayerPosition == 1 && fPlayerX <= 14.0f) || (nPlayerPosition == 2 && fPlayerX <= 38.0f)) {
            moveDirection = None;
            nPlayerPosition--;
            fPlayerX = (nPlayerPosition == 0) ? 14.0f : 38.0f;
        }
        Sleep(30); // 移動速度調整のための遅延
    }

    if (moveDirection == Right) {
        fPlayerX += 1.0f;
        if ((nPlayerPosition == 0 && fPlayerX >= 38.0f) || (nPlayerPosition == 1 && fPlayerX >= 62.0f)) {
            moveDirection = None;
            nPlayerPosition++;
            fPlayerX = (nPlayerPosition == 1) ? 38.0f : 62.0f;
        }
        Sleep(30); // 移動速度調整のための遅延
    }

    // プレイヤーの描画
    DrawPlayer(screen, screenColor);

    if (moveDirection == None) {
        // ステージの選択
        if (GetAsyncKeyState(VK_RETURN) & 0x8000 || inport(0x0203) == 2 || inport(0x0213) == 2 || inport(0x0223) == 2 || inport(0x0233) == 2) {
            switch (nPlayerPosition) {
            case 0:
                StopBGM();
                currentScene = Draw0Scene;
                for (int y = 0; y < nScreenHeight; y++) {
                    for (int x = 0; x < nScreenWidth; x++) {
                        screenColor[y * nScreenWidth + x] = FG_BLACK | BG_BLACK;
                    }
                }
                {
                    std::wstring stageLabel = L"Tutorial";
                    int labelStart = 40 - stageLabel.size() / 2;
                    for (size_t i = 0; i < stageLabel.size(); i++) {
                        screen[nScreenWidth * ((nScreenHeight / 2)) + labelStart + i] = stageLabel[i];
                        screenColor[nScreenWidth * ((nScreenHeight / 2)) + labelStart + i] = FG_WHITE | BG_BLACK;
                    }
                }
                break;
            case 1:
                StopBGM();
                currentScene = Draw1Scene;
                for (int y = 0; y < nScreenHeight; y++) {
                    for (int x = 0; x < nScreenWidth; x++) {
                        screenColor[y * nScreenWidth + x] = FG_BLACK | BG_BLACK;
                    }
                }
                {
                    std::wstring stageLabel = L"Normal";
                    int labelStart = 40 - stageLabel.size() / 2;
                    for (size_t i = 0; i < stageLabel.size(); i++) {
                        screen[nScreenWidth * ((nScreenHeight / 2)) + labelStart + i] = stageLabel[i];
                        screenColor[nScreenWidth * ((nScreenHeight / 2)) + labelStart + i] = FG_WHITE | BG_BLACK;
                    }
                }
                break;
            case 2:
                StopBGM();
                currentScene = Draw2Scene;
                for (int y = 0; y < nScreenHeight; y++) {
                    for (int x = 0; x < nScreenWidth; x++) {
                        screenColor[y * nScreenWidth + x] = FG_BLACK | BG_BLACK;
                    }
                }
                {
                    std::wstring stageLabel = L"Hard";
                    int labelStart = 40 - stageLabel.size() / 2;
                    for (size_t i = 0; i < stageLabel.size(); i++) {
                        screen[nScreenWidth * ((nScreenHeight / 2)) + labelStart + i] = stageLabel[i];
                        screenColor[nScreenWidth * ((nScreenHeight / 2)) + labelStart + i] = FG_WHITE | BG_BLACK;
                    }
                }
                break;
            }
        }
    }
}

void Draw0(wchar_t* screen, WORD* screenColor) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    currentScene = TutorialScene;
    InitializeStageFromString();
    InitPlayer();
}

void Draw1(wchar_t* screen, WORD* screenColor) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    currentScene = Stage1_1Scene;
    InitializeStageFromString();
    InitPlayer();
}

void Draw2(wchar_t* screen, WORD* screenColor) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    currentScene = Stage1_2Scene;
    InitializeStageFromString();
    InitPlayer();
}

// プレイヤーを描画
void DrawPlayer(wchar_t* screen, WORD* screenColor) {
#pragma region player
    // プレイヤーの表示　5x3サイズ
    int px = (int)fPlayerX - (int)fCameraX;
    int pz = (int)fPlayerZ;
    screen[(pz - 2) * nScreenWidth + px] = 0x2585;
    screen[(pz - 2) * nScreenWidth + px + 1] = 0x2585;
    screen[(pz - 2) * nScreenWidth + px + 2] = 0x2585;
    screen[(pz - 2) * nScreenWidth + px + 3] = 0x2585;
    screen[(pz - 2) * nScreenWidth + px + 4] = 0x2585;

    screen[(pz - 1) * nScreenWidth + px] = 0x2585;
    screen[(pz - 1) * nScreenWidth + px + 1] = 0x2585;
    screen[(pz - 1) * nScreenWidth + px + 2] = 0x2585;
    screen[(pz - 1) * nScreenWidth + px + 3] = 0x2585;
    screen[(pz - 1) * nScreenWidth + px + 4] = 0x2585;

    screen[(pz)*nScreenWidth + px] = 0x2585;
    screen[(pz)*nScreenWidth + px + 1] = 0x2585;
    screen[(pz)*nScreenWidth + px + 2] = 0x2585;
    screen[(pz)*nScreenWidth + px + 3] = 0x2585;
    screen[(pz)*nScreenWidth + px + 4] = 0x2585;

    // プレイヤーの色設定
    WORD playerColor1 = FG_BROWN | BG_RED;
    WORD playerColor2 = FG_YELLOW | BG_RED;
    WORD playerColor3 = FG_BLACK | BG_RED;
    WORD playerColor4 = FG_WHITE | BG_RED;
    if (currentScene != SelectScene) {
        if (isStage1) {
            playerColor4 = FG_SKY | BG_RED;
        } else {
            playerColor4 = FG_GRAY | BG_RED;
        }
    }
    WORD playerColor5 = FG_WHITE | BG_RED;
    WORD playerColor6 = FG_BLUE | BG_BLUE;
    WORD playerColor7 = FG_BLUE | BG_RED;
    WORD playerColor8 = FG_BROWN | BG_WHITE;
    if (currentScene != SelectScene) {
        if (isStage1) {
            playerColor8 = FG_BROWN | BG_SKY;
        } else {
            playerColor8 = FG_BROWN | BG_GRAY;
        }
    }
    WORD playerColor9 = FG_BROWN | BG_BLUE;
    WORD playerColor10 = FG_WHITE | BG_BLUE;
    if (currentScene != SelectScene) {
        if (isStage1) {
            playerColor10 = FG_SKY | BG_BLUE;
        } else {
            playerColor10 = FG_GRAY | BG_BLUE;
        }
    }
    screenColor[(pz - 2) * nScreenWidth + px] = playerColor1;
    screenColor[(pz - 2) * nScreenWidth + px + 1] = playerColor2;
    screenColor[(pz - 2) * nScreenWidth + px + 2] = playerColor2;
    screenColor[(pz - 2) * nScreenWidth + px + 3] = playerColor3;
    screenColor[(pz - 2) * nScreenWidth + px + 4] = playerColor4;

    screenColor[(pz - 1) * nScreenWidth + px] = playerColor5;
    screenColor[(pz - 1) * nScreenWidth + px + 1] = playerColor6;
    screenColor[(pz - 1) * nScreenWidth + px + 2] = playerColor7;
    screenColor[(pz - 1) * nScreenWidth + px + 3] = playerColor6;
    screenColor[(pz - 1) * nScreenWidth + px + 4] = playerColor5;

    screenColor[pz * nScreenWidth + px] = playerColor8;
    screenColor[pz * nScreenWidth + px + 1] = playerColor9;
    screenColor[pz * nScreenWidth + px + 2] = playerColor10;
    screenColor[pz * nScreenWidth + px + 3] = playerColor9;
    screenColor[pz * nScreenWidth + px + 4] = playerColor8;
#pragma endregion
}

// 2D描画
void Draw2D(wchar_t* screen, WORD* screenColor) {
    UpdateCameraPosition();

    // Yが1のマップを描画
    for (int z = 0; z < nMapDepth; z++) {
        for (int x = 0; x < nScreenWidth; x++) {
            int mapX = (int)fCameraX + x;
            if (mapX >= 0 && mapX < nMapWidth) {
                char tile = isStage1 ? stage1[z][1][mapX] : stage2[z][1][mapX];
                screen[1 * nScreenWidth + x] = tile;
                // タイルに応じた色設定
                switch (tile) {
                case '#': screenColor[z * nScreenWidth + x] = FG_SKY | BG_SKY; break;
                case '=': screenColor[z * nScreenWidth + x] = FG_GREEN | BG_GREEN; break;
                case '-': screenColor[z * nScreenWidth + x] = FG_SKY | BG_SKY; break;
                case '.': screenColor[z * nScreenWidth + x] = FG_SKY | BG_SKY; break;
                case '%': screenColor[z * nScreenWidth + x] = FG_BLACK_GRAY | BG_BLACK_GRAY; break;
                case '@': screenColor[z * nScreenWidth + x] = FG_BRIGHT_GREEN | BG_BRIGHT_GREEN; break;
                case '*': screenColor[z * nScreenWidth + x] = FG_BRIGHT_GREEN | BG_BRIGHT_GREEN; break;
                case 'I': screenColor[z * nScreenWidth + x] = FG_WHITE | BG_WHITE; break;
                case 'Y': screenColor[z * nScreenWidth + x] = FG_YELLOW | BG_YELLOW; break;
                case 'B': screenColor[z * nScreenWidth + x] = FG_BLACK_GRAY | BG_BLACK_GRAY; break;
                case 'b': screenColor[z * nScreenWidth + x] = FG_BLACK_GRAY | BG_BLACK_GRAY; break;
                case 'F': screenColor[z * nScreenWidth + x] = FG_RED | BG_RED; break;
                case 'f': screenColor[z * nScreenWidth + x] = FG_BROWN | BG_BROWN; break;
                case 'R':
                    screen[z * nScreenWidth + x] = 0x2635;
                    screenColor[z * nScreenWidth + x] = FG_BLACK | BG_BROWN;
                    break;
                case 'c':
                    if (isStage1) {
                        screen[z * nScreenWidth + x] = 0x2B24;
                        screenColor[z * nScreenWidth + x] = FG_YELLOW | BG_SKY;
                    } else {
                        screen[z * nScreenWidth + x] = 0x2B24;
                        screenColor[z * nScreenWidth + x] = FG_YELLOW | BG_GRAY;
                    }
                    break;
                case 'S':
                    if (isStage1) {
                        screen[z * nScreenWidth + x] = 0x22C6;
                        screenColor[z * nScreenWidth + x] = FG_RED | BG_SKY;
                    } else {
                        screen[z * nScreenWidth + x] = 0x22C6;
                        screenColor[z * nScreenWidth + x] = FG_RED | BG_GRAY;
                    }
                    break;
                case '_': screenColor[z * nScreenWidth + x] = FG_SKY | BG_SKY; break;

                case '!': screenColor[z * nScreenWidth + x] = FG_RED | BG_RED; break;
                case '~': screenColor[z * nScreenWidth + x] = FG_BLACK | BG_BLACK; break;
                case '>': screenColor[z * nScreenWidth + x] = FG_GRAY | BG_GRAY; break;
                case ':': screenColor[z * nScreenWidth + x] = FG_BLACK | BG_BLACK; break;

                case 'K': screenColor[z * nScreenWidth + x] = FG_BLUE | BG_BLUE; break;
                default: screenColor[z * nScreenWidth + x] = FG_WHITE | BG_BLACK; break;

                }
            } else {
                screen[z * nScreenWidth + x] = ' ';
                screenColor[z * nScreenWidth + x] = FG_BLACK;
            }
        }
    }

    // Yが2から8までのオブジェクトを描画
    for (int y = 2; y <= 8; y++) {
        for (int z = 0; z < nMapDepth; z++) {
            for (int x = 0; x < nScreenWidth; x++) {
                int mapX = (int)fCameraX + x;
                if (mapX >= 0 && mapX < nMapWidth) {
                    char tile = isStage1 ? stage1[z][y][mapX] : stage2[z][y][mapX];
                    if (tile != '-' && tile != '.' && tile != '>') { // オーバーレイするタイルのみ描画
                        screen[y * nScreenWidth + x] = tile;
                        // タイルに応じた色設定
                        switch (tile) {
                        case '#': screenColor[z * nScreenWidth + x] = FG_SKY | BG_SKY; break;
                        case '=': screenColor[z * nScreenWidth + x] = FG_GREEN | BG_GREEN; break;
                        case '%': screenColor[z * nScreenWidth + x] = FG_BLACK_GRAY | BG_BLACK_GRAY; break;
                        case '@': screenColor[z * nScreenWidth + x] = FG_BRIGHT_GREEN | BG_BRIGHT_GREEN; break;
                        case '*': screenColor[z * nScreenWidth + x] = FG_BRIGHT_GREEN | BG_BRIGHT_GREEN; break;
                        case 'I': screenColor[z * nScreenWidth + x] = FG_WHITE | BG_WHITE; break;
                        case 'Y': screenColor[z * nScreenWidth + x] = FG_YELLOW | BG_YELLOW; break;
                        case 'B': screenColor[z * nScreenWidth + x] = FG_BLACK_GRAY | BG_BLACK_GRAY; break;
                        case 'b': screenColor[z * nScreenWidth + x] = FG_BLACK_GRAY | BG_BLACK_GRAY; break;
                        case 'F': screenColor[z * nScreenWidth + x] = FG_RED | BG_RED; break;
                        case 'f': screenColor[z * nScreenWidth + x] = FG_BROWN | BG_BROWN; break;
                        case 'R':
                            screen[z * nScreenWidth + x] = 0x2635;
                            screenColor[z * nScreenWidth + x] = FG_BLACK | BG_BROWN;
                            break;
                        case 'c':
                            if (isStage1) {
                                screen[z * nScreenWidth + x] = 0x2B24;
                                screenColor[z * nScreenWidth + x] = FG_YELLOW | BG_SKY;
                            } else {
                                screen[z * nScreenWidth + x] = 0x2B24;
                                screenColor[z * nScreenWidth + x] = FG_YELLOW | BG_GRAY;
                            }
                            break;
                        case 'S':
                            if (isStage1) {
                                screen[z * nScreenWidth + x] = 0x22C6;
                                screenColor[z * nScreenWidth + x] = FG_RED | BG_SKY;
                            } else {
                                screen[z * nScreenWidth + x] = 0x22C6;
                                screenColor[z * nScreenWidth + x] = FG_RED | BG_GRAY;
                            }
                            break;
                        case '_': screenColor[z * nScreenWidth + x] = FG_SKY | BG_SKY; break;

                        case '!': screenColor[z * nScreenWidth + x] = FG_RED | BG_RED; break;
                        case '~': screenColor[z * nScreenWidth + x] = FG_BLACK | BG_BLACK; break;
                        case ':': screenColor[z * nScreenWidth + x] = FG_BLACK | BG_BLACK; break;

                        case 'K': screenColor[z * nScreenWidth + x] = FG_BLUE | BG_BLUE; break;
                        default: screenColor[z * nScreenWidth + x] = FG_WHITE | BG_BLACK; break;
                        }
                    }
                }
            }
        }
    }

    // バーナーの描画
    if (isBurnerActive) {
        for (int z = 0; z < nMapDepth; z++) {
            for (int y = 0; y < nMapHeight; y++) {
                for (int x = 0; x < nMapWidth; x++) {
                    if (isStage1 ? stage1[z][y][x] == 'B' : stage2[z][y][x] == 'B') {
                        for (int i = 1; i <= 3; i++) {
                            isStage1 ? stage1[z - i][y][x] = 'F' : stage2[z - i][y][x] = 'F'; // 炎を描画
                        }
                    }
                }
            }
        }
        for (int z = 0; z < nMapDepth; z++) {
            for (int y = 0; y < nMapHeight; y++) {
                for (int x = 0; x < nMapWidth; x++) {
                    if (isStage1 ? stage1[z][y][x] == 'b' : stage2[z][y][x] == 'b') {
                        for (int i = 1; i <= 3; i++) {
                            isStage1 ? stage1[z - i][y][x] = '.' : stage2[z - i][y][x] = '.';
                        }
                    }
                }
            }
        }
    } else {
        for (int z = 0; z < nMapDepth; z++) {
            for (int y = 0; y < nMapHeight; y++) {
                for (int x = 0; x < nMapWidth; x++) {
                    if (isStage1 ? stage1[z][y][x] == 'B' : stage2[z][y][x] == 'B') {
                        for (int i = 1; i <= 3; i++) {
                            isStage1 ? stage1[z - i][y][x] = '.' : stage2[z - i][y][x] = '.'; // 炎を消す
                        }
                    }
                }
            }
        }
        for (int z = 0; z < nMapDepth; z++) {
            for (int y = 0; y < nMapHeight; y++) {
                for (int x = 0; x < nMapWidth; x++) {
                    if (isStage1 ? stage1[z][y][x] == 'b' : stage2[z][y][x] == 'b') {
                        for (int i = 1; i <= 3; i++) {
                            isStage1 ? stage1[z - i][y][x] = 'F' : stage2[z - i][y][x] = 'F'; // 炎を消す
                        }
                    }
                }
            }
        }
    }

    DrawPlayer(screen, screenColor);

    // 土管表示
    for (int y = 0; y < nMapHeight; y++) {
        for (int z = 0; z < nMapDepth; z++) {
            for (int x = 0; x < nScreenWidth; x++) {
                int mapX = (int)fCameraX + x;
                if (mapX >= 0 && mapX < nMapWidth) {
                    char tile = isStage1 ? stage1[z][y][mapX] : stage2[z][y][mapX];
                    // タイルに応じた色設定
                    switch (tile) {
                    case '@':
                        screen[y * nScreenWidth + x] = tile; screenColor[z * nScreenWidth + x] = FG_BRIGHT_GREEN | BG_BRIGHT_GREEN; break;
                    case '*':
                        screen[y * nScreenWidth + x] = tile; screenColor[z * nScreenWidth + x] = FG_BRIGHT_GREEN | BG_BRIGHT_GREEN; break;
                    }

                }
            }
        }
    }

}

// Draw3D関数の実装
void Draw3D(wchar_t* screen, WORD* screenColor) {
    auto getColorForTile = [](char tile) -> WORD {
        switch (tile) {
        case '#': return FG_SKY | BG_SKY;
        case '=': return FG_GREEN | BG_GREEN;
        case '-': return FG_SKY | BG_SKY;
        case '.': return FG_SKY | BG_SKY;
        case '%': return FG_BLACK_GRAY | BG_BLACK_GRAY;
        case '@': return FG_BRIGHT_GREEN | BG_BRIGHT_GREEN;
        case '*': return FG_BRIGHT_GREEN | BG_BRIGHT_GREEN;
        case 'I': return FG_WHITE | BG_WHITE;
        case 'Y': return FG_YELLOW | BG_YELLOW;
        case 'B': return FG_BLACK_GRAY | BG_BLACK_GRAY;
        case 'b': return FG_BLACK_GRAY | BG_BLACK_GRAY;
        case 'F': return FG_RED | BG_RED;
        case 'f': return FG_BROWN | BG_BROWN;
        case 'R': return FG_BLACK | BG_BROWN;
        case 'c':
            if (isStage1) {
                return FG_YELLOW | BG_SKY;
            } else {
                return FG_YELLOW | BG_GRAY;
            }
        case 'S':
            if (isStage1) {
                return FG_RED | BG_SKY;
            } else {
                return FG_RED | BG_GRAY;
            }
        case '_': return FG_BLACK | BG_BLACK;

        case '!': return FG_RED | BG_RED;
        case '~': return FG_BLACK | BG_BLACK;
        case '>': return FG_GRAY | BG_GRAY;
        case ':': return FG_BLACK | BG_BLACK;

        case 'K': return FG_BLUE | BG_BLUE;
        default: return FG_BLACK | FG_BLACK;
        }
        };

    auto getShadeForTile = [](char tile) -> short {
        switch (tile) {
        case '#': return 0x2588;
        case '=': return 0x2588;
        case '-': return 0x2588;
        case '.': return 0x2588;
        case '%': return 0x2588;
        case '@': return 0x2588;
        case '*': return 0x2588;
        case 'I': return 0x2588;
        case 'Y': return 0x2588;
        case 'B': return 0x2588;
        case 'b': return 0x2588;
        case 'F': return 0x2588;
        case 'f': return 0x2588;
        case 'R': return 0x2635;
        case 'c': return 0x2B24;
        case 'S': return 0x22C6;
        case '_': return 0x2588;
        case '!': return 0x2588;
        case '~': return 0x2588;
        case '>': return 0x2588;
        case ':': return 0x2588;
        case 'K': return 0x2588;
        default: return ' ';
        }
        };

    // 垂直方向の視点角度を少し下げる
    float fVerticalAngleOffset = 10.0f * (3.14159f / 180.0f); // 10度下げる

    for (int x = 0; x < nScreenWidth; x++) {
        for (int y = 0; y < nScreenHeight; y++) {
            float fRayAngleH = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;
            float fRayAngleV = ((float)y / (float)nScreenHeight) * fFOV - (fFOV / 2.0f) + fVerticalAngleOffset;

            float fStepSize = 0.1f;

            float fDistanceToWall = 0.0f;
            bool bHitWall = false;
            bool bBoundary = false;

            float fEyeX = (cosf(fRayAngleH) * cosf(fRayAngleV)) * 3;
            float fEyeY = (sinf(fRayAngleH) * cosf(fRayAngleV)) * 2;
            float fEyeZ = (sinf(fRayAngleV)) * 2;

            char tileHit = ' ';
            std::vector<std::tuple<float, char, bool, int>> wallSegments;

            while (!bHitWall && fDistanceToWall < fDepth) {
                fDistanceToWall += fStepSize;
                int nTestX = (int)((fPlayerX - 10.0f) + fEyeX * fDistanceToWall);   // カメラ座標の調整
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);
                int nTestZ = (int)((fPlayerZ - 3.0f) + fEyeZ * fDistanceToWall);

                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight || nTestZ < 0 || nTestZ >= nMapDepth) {
                    bHitWall = true;
                    fDistanceToWall = fDepth;
                } else {
                    tileHit = isStage1 ? stage1[nTestZ][nTestY][nTestX] : stage2[nTestZ][nTestY][nTestX];
                    if (tileHit == '#' || tileHit == '=' || tileHit == '-' || tileHit == '%' || tileHit == '@' || tileHit == '*' || tileHit == 'I' || tileHit == 'F' || tileHit == 'f' || tileHit == 'B' || tileHit == 'b' || tileHit == 'R' || tileHit == 'c' || tileHit == 'S' || tileHit == '_' || tileHit == '!' || tileHit == 'K' || tileHit == ':') {
                        bHitWall = true;

                        std::vector<std::pair<float, float>> p;
                        for (int tx = 0; tx < 2; tx++) {
                            for (int ty = 0; ty < 2; ty++) {
                                for (int tz = 0; tz < 2; tz++) {
                                    float vy = (float)nTestY + ty - fPlayerY;
                                    float vx = (float)nTestX + tx - fPlayerX;
                                    float vz = (float)nTestZ + tz - fPlayerZ;
                                    float d = sqrt(vx * vx + vy * vy + vz * vz);
                                    if (d != 0.0f) {  // ゼロ除算を防止
                                        float dot = (fEyeX * vx / d) + (fEyeY * vy / d) + (fEyeZ * vz / d);
                                        p.push_back(std::make_pair(d, dot));
                                    }
                                }
                            }
                        }

                        std::sort(p.begin(), p.end(), [](const std::pair<float, float>& left, const std::pair<float, float>& right) {
                            return left.first < right.first;
                            });

                        float fBound = 0.00f;
                        if (p.size() >= 3) {
                            if (fabs(p.at(0).second) < fBound) bBoundary = true;
                            if (fabs(p.at(1).second) < fBound) bBoundary = true;
                            if (fabs(p.at(2).second) < fBound) bBoundary = true;
                        }
                        wallSegments.push_back(std::make_tuple(fDistanceToWall, tileHit, bBoundary, nTestZ));
                    }
                }
            }

            for (const auto& segment : wallSegments) {
                float segmentDistance;
                char segmentTile;
                bool segmentBoundary;
                int segmentZ;
                std::tie(segmentDistance, segmentTile, segmentBoundary, segmentZ) = segment;

                int nCeiling = (float)(nScreenHeight / 20.0) - nScreenHeight / ((float)segmentDistance);    // シェードする高さ
                int nFloor = nScreenHeight - nCeiling;

                short nShade = getShadeForTile(segmentTile);
                WORD nColor = getColorForTile(segmentTile);

                if (segmentBoundary) {
                    nShade = ' ';
                    nColor = FG_BLACK;
                }

                // 床と天井の描画
                if (y <= nCeiling) {
                    screen[y * nScreenWidth + x] = ' ';
                    screenColor[y * nScreenWidth + x] = FG_BLACK;
                } else if (y > nCeiling && y <= nFloor) {
                    screen[y * nScreenWidth + x] = nShade;
                    screenColor[y * nScreenWidth + x] = nColor;
                } else {
                    float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                    if (b < 0.25f) {
                        nShade = '#';
                        nColor = ((FOREGROUND_GREEN | FOREGROUND_INTENSITY) | (BACKGROUND_GREEN | BACKGROUND_INTENSITY));
                    } else if (b < 0.5f) {
                        nShade = 'x';
                        nColor = ((FOREGROUND_GREEN | FOREGROUND_INTENSITY) | (BACKGROUND_GREEN | BACKGROUND_INTENSITY));
                    } else if (b < 0.75f) {
                        nShade = '.';
                        nColor = FOREGROUND_GREEN | BACKGROUND_GREEN;
                    } else if (b < 0.9f) {
                        nShade = '-';
                        nColor = FOREGROUND_GREEN | BACKGROUND_GREEN;
                    } else {
                        nShade = ' ';
                        nColor = FG_BLACK;
                    }
                    screen[y * nScreenWidth + x] = nShade;
                    screenColor[y * nScreenWidth + x] = nColor;
                }
            }
            // 空間の描画
            if (!bHitWall) {
                screen[y * nScreenWidth + x] = ' ';
                if (isStage1) {
                    screenColor[y * nScreenWidth + x] = FG_SKY | BG_SKY;
                } else {
                    screenColor[y * nScreenWidth + x] = FG_GRAY | BG_GRAY;
                }
            }
        }
    }

    // バーナーの描画
    if (isBurnerActive) {
        for (int z = 0; z < nMapDepth; z++) {
            for (int y = 0; y < nMapHeight; y++) {
                for (int x = 0; x < nMapWidth; x++) {
                    if (isStage1 ? stage1[z][y][x] == 'B' : stage2[z][y][x] == 'B') {
                        for (int i = 1; i <= 3; i++) {
                            isStage1 ? stage1[z - i][y][x] = 'F' : stage2[z - i][y][x] = 'F'; // 炎を描画
                        }
                    }
                }
            }
        }
        for (int z = 0; z < nMapDepth; z++) {
            for (int y = 0; y < nMapHeight; y++) {
                for (int x = 0; x < nMapWidth; x++) {
                    if (isStage1 ? stage1[z][y][x] == 'b' : stage2[z][y][x] == 'b') {
                        for (int i = 1; i <= 3; i++) {
                            isStage1 ? stage1[z - i][y][x] = '.' : stage2[z - i][y][x] = '.';
                        }
                    }
                }
            }
        }
    } else {
        for (int z = 0; z < nMapDepth; z++) {
            for (int y = 0; y < nMapHeight; y++) {
                for (int x = 0; x < nMapWidth; x++) {
                    if (isStage1 ? stage1[z][y][x] == 'B' : stage2[z][y][x] == 'B') {
                        for (int i = 1; i <= 3; i++) {
                            isStage1 ? stage1[z - i][y][x] = '.' : stage2[z - i][y][x] = '.'; // 炎を消す
                        }
                    }
                }
            }
        }
        for (int z = 0; z < nMapDepth; z++) {
            for (int y = 0; y < nMapHeight; y++) {
                for (int x = 0; x < nMapWidth; x++) {
                    if (isStage1 ? stage1[z][y][x] == 'b' : stage2[z][y][x] == 'b') {
                        for (int i = 1; i <= 3; i++) {
                            isStage1 ? stage1[z - i][y][x] = 'F' : stage2[z - i][y][x] = 'F'; // 炎を消す
                        }
                    }
                }
            }
        }
    }


    // プレイヤーを描画
    int nPlayerScreenX = (nScreenWidth / 2) - 2;
    int nPlayerScreenY = (nScreenHeight / 2) + 3;

    // プレイヤーの色設定
    WORD playerColor1 = FG_BROWN | BG_RED;
    WORD playerColor2 = FG_YELLOW | BG_RED;
    WORD playerColor4;
    if (isStage1) {
        playerColor4 = FG_SKY | BG_RED;
    } else {
        playerColor4 = FG_GRAY | BG_RED;
    }
    WORD playerColor5 = FG_WHITE | BG_RED;
    WORD playerColor6 = FG_BLUE | BG_BLUE;
    WORD playerColor7 = FG_BLUE | BG_RED;
    WORD playerColor8;
    if (isStage1) {
        playerColor8 = FG_BROWN | BG_SKY;
    } else {
        playerColor8 = FG_BROWN | BG_GRAY;
    }
    WORD playerColor9 = FG_BROWN | BG_BLUE;
    WORD playerColor10;
    if (isStage1) {
        playerColor10 = FG_SKY | BG_BLUE;
    } else {
        playerColor10 = FG_GRAY | BG_BLUE;
    }
    int screenX = nPlayerScreenX;
    int screenY = nPlayerScreenY;
    if (screenX >= 0 && screenX < nScreenWidth && screenY >= 0 && screenY < nScreenHeight) {
        for (int i = 0; i <= 2; i++) {
            for (int j = 0; j < 5; j++) {
                screen[(screenY + i) * nScreenWidth + screenX + j] = 0x2585;
            }
        }
        screenColor[screenY * nScreenWidth + screenX] = playerColor2;
        screenColor[screenY * nScreenWidth + screenX + 1] = playerColor1;
        screenColor[screenY * nScreenWidth + screenX + 2] = playerColor1;
        screenColor[screenY * nScreenWidth + screenX + 3] = playerColor1;
        screenColor[screenY * nScreenWidth + screenX + 4] = playerColor2;

        screenColor[(screenY + 1) * nScreenWidth + screenX] = playerColor5;
        screenColor[(screenY + 1) * nScreenWidth + screenX + 1] = playerColor6;
        screenColor[(screenY + 1) * nScreenWidth + screenX + 2] = playerColor7;
        screenColor[(screenY + 1) * nScreenWidth + screenX + 3] = playerColor6;
        screenColor[(screenY + 1) * nScreenWidth + screenX + 4] = playerColor5;

        screenColor[(screenY + 2) * nScreenWidth + screenX] = playerColor8;
        screenColor[(screenY + 2) * nScreenWidth + screenX + 1] = playerColor9;
        screenColor[(screenY + 2) * nScreenWidth + screenX + 2] = playerColor10;
        screenColor[(screenY + 2) * nScreenWidth + screenX + 3] = playerColor9;
        screenColor[(screenY + 2) * nScreenWidth + screenX + 4] = playerColor8;
    }

    // デバッグ時に座標を表示
#ifdef _DEBUG
    swprintf(screen, 40, L"X: %3.2f Y: %3.2f Z: %3.2f", fPlayerX, fPlayerY, fPlayerZ);
#endif
}

// 最後のキー入力時刻を記録する変数
auto lastToggleTime = std::chrono::steady_clock::now();

// Playerのキー入力　　　横の当たり判定だけは、ここで行う
void PlayerMovement(wchar_t* screen, WORD* screenColor, HANDLE hScreen) {
    // 現在の時刻を取得
    auto nowTime = std::chrono::steady_clock::now();
    // 2D_3D切り替え
    if ((GetAsyncKeyState((unsigned short)'M') & 0x8000) || inport(0x0208) == 0 || inport(0x0218) == 0 || inport(0x0228) == 0 || inport(0x0238) == 0) {
        // 前回の切り替えからの経過時間を計算
        auto moveChangeCheakTime = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastToggleTime).count();

        // クールタイム800秒
        if (moveChangeCheakTime >= 800) {
            is2DMode = !is2DMode;
            lastToggleTime = nowTime; // 最後のキー入力時刻を更新
        }
    }

    // 土管での移動処理
    auto checkPipeCollision = [&]() -> bool {
        if (is2DMode) {
            for (int y = 0; y < nMapHeight; y++) {
                if (isStage1 ? stage1[(int)fPlayerZ + 1][y][(int)fPlayerX + 2] == '@' : stage2[(int)fPlayerZ + 1][y][(int)fPlayerX + 2] == '@') {
                    return true;
                }
            }
            return false;
        } else {
            return isStage1 ? stage1[(int)fPlayerZ + 1][(int)fPlayerY][(int)fPlayerX + 2] == '@' : stage2[(int)fPlayerZ + 1][(int)fPlayerY][(int)fPlayerX + 2] == '@';
        }
        };

    if (is2DMode) { // 2D
        bool collision = false;
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000 || inport(0x0200) < 31000 || inport(0x0210) < 31000 || inport(0x0220) < 31000 || inport(0x0230) < 31000) {
            for (int y = 1; y <= 8; y++) {
                for (int i = 0; i < playerHeight; i++) {  // 左側の当たり判定
                    if (isStage1 ?
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '#' || stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '@' ||
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '*' || stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == 'B' ||
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == 'R' || stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '%' ||
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '~' || stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '=' ||
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == 'K' || stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == ':' ||
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == 'b' :

                    stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '#' || stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '@' ||
                        stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '*' || stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == 'B' ||
                        stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == 'R' || stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '%' ||
                        stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '~' || stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == '=' ||
                        stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == 'K' || stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == ':' ||
                        stage2[(int)fPlayerZ - i][y][(int)fPlayerX - 1] == 'b') {
                        collision = true;
                        break;
                    }
                }
            }
            if (!collision && fPlayerX > fPlayerXMAX) fPlayerX -= fSpeed * fElapsedTime; // X軸6以下へ進めないようにする
        }
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000 || inport(0x0200) > 34000 || inport(0x0210) > 34000 || inport(0x0220) > 34000 || inport(0x0230) > 34000) {
            for (int y = 1; y <= 8; y++) {
                for (int i = 0; i < playerHeight; i++) {  // 右側の当たり判定
                    if (isStage1 ?
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '#' || stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '@' ||
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '*' || stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == 'B' ||
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == 'R' || stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '%' ||
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '~' || stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '=' ||
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == 'K' || stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == ':' ||
                        stage1[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == 'b':

                    stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '#' || stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '@' ||
                        stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '*' || stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == 'B' ||
                        stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == 'R' || stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '%' ||
                        stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '~' || stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == '=' ||
                        stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == 'K' || stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == ':' ||
                        stage2[(int)fPlayerZ - i][y][(int)fPlayerX + playerWidth] == 'b') {
                        collision = true;
                        break;
                    }
                }
            }
            if (!collision) fPlayerX += fSpeed * fElapsedTime;
        }
        if (GetAsyncKeyState(VK_SPACE) & 0x8000 || inport(0x0203) == 1 || inport(0x0213) == 1 || inport(0x0223) == 1 || inport(0x0233) == 1) {
            if (!isJumping && !isFalling) {
                isJumping = true;
                jumpVelocity = jumpSpeed;
                PlaySoundEffect(L"Sounds/jump.wav", 30);
            }
        }

        // 土管移動（2D）
        if (checkPipeCollision() && ((GetAsyncKeyState((unsigned short)'S') & 0x8000 || GetAsyncKeyState(VK_SHIFT) & 0x8000) || inport(0x0208) == 18000 || inport(0x0218) == 18000 || inport(0x0228) == 18000 || inport(0x0238) == 18000 || inport(0x0201) > 54000 || inport(0x0211) > 54000 || inport(0x0221) > 54000 || inport(0x0231) > 54000)) {
            // 現在の座標を維持してstageMap2に移動
            SwitchStage(screen, screenColor, hScreen);
            return;
        }

        HandlePhysics(fElapsedTime);

        if (CheckGoalCondition()) {
            isGoalReached = true;
        }
    } else {    // 3D
        if (GetAsyncKeyState((unsigned short)'W') & 0x8000 || inport(0x0201) < 25000 || inport(0x0211) < 25000 || inport(0x0221) < 25000 || inport(0x0231) < 25000) {
            float fNewPlayerX = fPlayerX + cosf(fPlayerA) * fSpeed * fElapsedTime;
            float fNewPlayerY = fPlayerY + sinf(fPlayerA) * fSpeed * fElapsedTime;
            bool collision = false;
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 1; j++) {
                    for (int k = 0; k < 3; k++) {
                        if (fNewPlayerX < 0 || fNewPlayerX >= nMapWidth || fNewPlayerY < 0 || fNewPlayerY >= nMapHeight ||
                            isStage1 ?
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '#' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '@' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '*' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'B' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'R' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '%' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '~' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '=' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'K' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == ':' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'b':

                        stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '#' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '@' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '*' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'B' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'R' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '%' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '~' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '=' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'K' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == ':' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'b') {
                            collision = true;
                            break;
                        }
                    }
                }
            }
            if (!collision && fNewPlayerX > fPlayerXMAX) { // X軸fPlayerXMAX以下へ進めないようにする
                fPlayerX = fNewPlayerX;
                fPlayerY = fNewPlayerY;
            }
        }
        if (GetAsyncKeyState((unsigned short)'S') & 0x8000 || inport(0x0201) > 40000 || inport(0x0211) > 40000 || inport(0x0221) > 40000 || inport(0x0231) > 40000) {
            float fNewPlayerX = fPlayerX - cosf(fPlayerA) * fSpeed * fElapsedTime;
            float fNewPlayerY = fPlayerY - sinf(fPlayerA) * fSpeed * fElapsedTime;
            bool collision = false;
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 1; j++) {
                    for (int k = 0; k < 3; k++) {
                        if (fNewPlayerX < 0 || fNewPlayerX >= nMapWidth || fNewPlayerY < 0 || fNewPlayerY >= nMapHeight ||
                            isStage1 ?
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '#' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '@' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '*' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'B' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'R' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '%' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '~' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '=' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'K' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == ':' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'b':

                        stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '#' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '@' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '*' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'B' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'R' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '%' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '~' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '=' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'K' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == ':' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'b') {
                            collision = true;
                            break;
                        }
                    }
                }
            }
            if (!collision && fNewPlayerX > fPlayerXMAX) { // X軸fPlayerXMAX以下へ進めないようにする
                fPlayerX = fNewPlayerX;
                fPlayerY = fNewPlayerY;
            }
        }
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000 || inport(0x0200) < 25000 || inport(0x0210) < 25000 || inport(0x0220) < 25000 || inport(0x0230) < 25000) {
            float fNewPlayerX = (fPlayerX + sinf(fPlayerA) * (fSpeed / 1.5) * fElapsedTime);
            float fNewPlayerY = (fPlayerY - cosf(fPlayerA) * (fSpeed / 1.5) * fElapsedTime);
            bool collision = false;
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 1; j++) {
                    for (int k = 0; k < 3; k++) {
                        if (fNewPlayerX < 0 || fNewPlayerX >= nMapWidth || fNewPlayerY < 0 || fNewPlayerY >= nMapHeight ||
                            isStage1 ?
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '#' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '@' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '*' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'B' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'R' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '%' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '~' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '=' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'K' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == ':' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'b':

                        stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '#' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '@' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '*' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'B' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'R' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '%' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '~' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '=' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'K' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == ':' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'b') {
                            collision = true;
                            break;
                        }
                    }
                }
            }
            if (!collision && fNewPlayerX > fPlayerXMAX) { // X軸fPlayerXMAX以下へ進めないようにする
                fPlayerX = fNewPlayerX;
                fPlayerY = fNewPlayerY;
            }
        }
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000 || inport(0x0200) > 40000 || inport(0x0210) > 40000 || inport(0x0220) > 40000 || inport(0x0230) > 40000) {
            float fNewPlayerX = (fPlayerX - sinf(fPlayerA) * (fSpeed / 1.5) * fElapsedTime);
            float fNewPlayerY = (fPlayerY + cosf(fPlayerA) * (fSpeed / 1.5) * fElapsedTime);
            bool collision = false;
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 1; j++) {
                    for (int k = 0; k < 3; k++) {

                        if (fNewPlayerX < 0 || fNewPlayerX >= nMapWidth || fNewPlayerY < 0 || fNewPlayerY >= nMapHeight ||
                            isStage1 ?
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '#' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '@' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '*' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'B' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'R' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '%' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '~' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '=' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'K' || stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == ':' ||
                            stage1[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'b':

                        stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '#' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '@' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '*' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'B' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'R' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '%' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '~' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == '=' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'K' || stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == ':' ||
                            stage2[(int)fPlayerZ - k][(int)fNewPlayerY + j][(int)fNewPlayerX + i] == 'b') {
                            collision = true;
                            break;
                        }
                    }
                }
            }
            if (!collision && fNewPlayerX > fPlayerXMAX) { // X軸fPlayerXMAX以下へ進めないようにする
                fPlayerX = fNewPlayerX;
                fPlayerY = fNewPlayerY;
            }
        }

        // 土管移動（3D）
        if (checkPipeCollision() && (GetAsyncKeyState(VK_SHIFT) & 0x8000 || inport(0x0208) == 18000 || inport(0x0218) == 18000 || inport(0x0228) == 18000 || inport(0x0238) == 18000)) {
            // 現在の座標を維持してstageMap2に移動
            SwitchStage(screen, screenColor, hScreen);
            return;
        }

        if (GetAsyncKeyState(VK_SPACE) & 0x8000 || inport(0x0203) == 1 || inport(0x0213) == 1 || inport(0x0223) == 1 || inport(0x0233) == 1) {
            if (!isJumping && !isFalling) {
                isJumping = true;
                jumpVelocity = jumpSpeed;
                PlaySoundEffect(L"Sounds/jump.wav", 30);
            }
        }

        HandlePhysics(fElapsedTime);

        if (CheckGoalCondition()) {
            isGoalReached = true;
        }
    }
}

// ジャンプとかの物理演算
void HandlePhysics(float fElapsedTime) {
    // Z軸が24以上になった場合、ゲームオーバーに移行（落下したら）
    if (fPlayerZ >= 24) {
        currentScene = GameOverScene;
        InitializeStageFromString();
        InitPlayer();
        isGameOver = true;
    }

    // 当たり判定
    auto checkCollision = [](float x, float y, float z) {
        return
            isStage1 ?
            stage1[(int)z][(int)y][(int)x] == '#' || stage1[(int)z][(int)y][(int)x] == '=' ||
            stage1[(int)z][(int)y][(int)x] == '-' || stage1[(int)z][(int)y][(int)x] == '*' ||
            stage1[(int)z][(int)y][(int)x] == 'R' || stage1[(int)z][(int)y][(int)x] == '@' ||
            stage1[(int)z][(int)y][(int)x] == 'B' || stage1[(int)z][(int)y][(int)x] == '%' ||
            stage1[(int)z][(int)y][(int)x] == '~' || stage1[(int)z][(int)y][(int)x] == 'K' ||
            stage1[(int)z][(int)y][(int)x] == ':' || stage1[(int)z][(int)y][(int)x] == 'b' :

        stage2[(int)z][(int)y][(int)x] == '#' || stage2[(int)z][(int)y][(int)x] == '=' ||
            stage2[(int)z][(int)y][(int)x] == '-' || stage2[(int)z][(int)y][(int)x] == '@' ||
            stage2[(int)z][(int)y][(int)x] == '*' || stage2[(int)z][(int)y][(int)x] == 'B' ||
            stage2[(int)z][(int)y][(int)x] == 'R' || stage2[(int)z][(int)y][(int)x] == '%' ||
            stage2[(int)z][(int)y][(int)x] == '~' || stage2[(int)z][(int)y][(int)x] == 'K' ||
            stage2[(int)z][(int)y][(int)x] == ':' || stage2[(int)z][(int)y][(int)x] == 'b';
        };

    if (isJumping) {
        fPlayerZ -= jumpVelocity * fElapsedTime;
        jumpVelocity -= gravity * fElapsedTime;

        if (is2DMode) {
            for (int y = 1; y <= 8; y++) {
                // プレイヤーがジャンプ中に天井にぶつかるかどうかを確認
                for (int i = 0; i < playerWidth; i++) {
                    if (checkCollision(fPlayerX + i, y, fPlayerZ - (playerHeight - 1))) {
                        isJumping = false;
                        isFalling = true;
                        jumpVelocity = 0;
                        fPlayerZ = ceil(fPlayerZ);
                        break;
                    }
                }
            }
        } else {
            // プレイヤーがジャンプ中に天井にぶつかるかどうかを確認
            for (int i = 0; i < playerWidth; i++) {
                if (checkCollision(fPlayerX + i, fPlayerY, fPlayerZ - (playerHeight - 1))) {
                    isJumping = false;
                    isFalling = true;
                    jumpVelocity = 0;
                    fPlayerZ = ceil(fPlayerZ);
                    break;
                }
            }
        }

        if (jumpVelocity <= 0) {
            isJumping = false;
            isFalling = true;
        }
    } else if (isFalling) {
        fPlayerZ += gravity * fElapsedTime;

        if (is2DMode) { // 2D
            for (int y = 1; y <= 8; y++) {
                // プレイヤーが落下中に床にぶつかるかどうかを確認
                for (int i = 0; i < playerWidth; i++) {
                    if (checkCollision(fPlayerX + i, y, fPlayerZ + 1)) {
                        fPlayerZ = floor(fPlayerZ);
                        isFalling = false;
                        break;
                    }
                }
            }
        } else {    // 3D
            // プレイヤーが落下中に床にぶつかるかどうかを確認
            for (int i = 0; i < playerWidth; i++) {
                if (checkCollision(fPlayerX + i, fPlayerY, fPlayerZ + 1)) {
                    fPlayerZ = floor(fPlayerZ);
                    isFalling = false;
                    break;
                }
            }
        }
    } else if (GetAsyncKeyState(VK_SPACE) & 0x8000 || inport(0x0203) == 1 || inport(0x0213) == 1 || inport(0x0223) == 1 || inport(0x0233) == 1) {
        if (is2DMode) { // 2D
            for (int y = 1; y <= 8; y++) {
                if (!checkCollision(fPlayerX, y, fPlayerZ - 1)) {
                    isJumping = true;
                    jumpVelocity = jumpSpeed;
                }
            }
        } else {    // 3D
            if (!checkCollision(fPlayerX, fPlayerY, fPlayerZ - 1)) {
                isJumping = true;
                jumpVelocity = jumpSpeed;
            }
        }
    }

    if (!isJumping && !isFalling) {
        bool collision = false;
        if (is2DMode) { // 2D
            for (int y = 1; y <= 8; y++) {
                for (int i = 0; i < playerWidth; i++) {
                    if (checkCollision(fPlayerX + i, y, fPlayerZ + 1)) {
                        collision = true;
                        break;
                    }
                }
            }
        } else {    // 3D
            for (int i = 0; i < playerWidth; i++) {
                if (checkCollision(fPlayerX + i, fPlayerY, fPlayerZ + 1)) {
                    collision = true;
                    break;
                }
            }
        }
        if (!collision) {
            isFalling = true;
        }
    }

#pragma region CollisionCheak
    // Rブロックとの衝突判定
    for (int i = 0; i < playerWidth; i++) {
        int px = (int)fPlayerX + i;
        int py = (int)fPlayerY;
        int pz = fPlayerZ;

        if (is2DMode) { // 2D
            for (int y = 1; y <= 8; y++) {   // Y軸1~8まで描画しているからそこだけチェック
                if (isStage1 ? stage1[pz - playerHeight][y][px] == 'R' : stage2[pz - playerHeight][y][px] == 'R') {
                    isStage1 ? stage1[pz - playerHeight][y][px] = '.' : stage2[pz - playerHeight][y][px] = '>';
                    fPlayerZ = fPlayerZ - 0.1;
                    isJumping = false;
                    isFalling = true;
                    jumpVelocity = 0;
                    PlaySoundEffect(L"Sounds/renga.wav", 30);
                }
            }
        } else {    // 3D
            if (isStage1 ? stage1[(int)pz - playerHeight][py][px] == 'R' : stage2[(int)pz - playerHeight][py][px] == 'R') {
                isStage1 ? stage1[(int)pz - playerHeight][py][px] = '.' : stage2[(int)pz - playerHeight][py][px] = '>';
                fPlayerZ = fPlayerZ - 0.1;
                isJumping = false;
                isFalling = true;
                jumpVelocity = 0;
                PlaySoundEffect(L"Sounds/renga.wav", 30);
            }
        }
    }

    // ホットスターとの衝突判定
    for (int i = 0; i < playerWidth; i++) {
        for (int j = 0; j < playerHeight; j++) {
            int px = (int)fPlayerX + i;
            int py = (int)fPlayerY;
            int pz = (int)fPlayerZ - j;

            if (is2DMode) { // 2D
                for (int y = 1; y <= 8; y++) {   // Y軸1~8まで描画しているからそこだけチェック
                    if (isStage1 ? stage1[pz][y][px] == 'S' : stage2[pz][y][px] == 'S') {
                        isStage1 ? stage1[pz][y][px] = '.' : stage2[pz][y][px] = '>';
                        hotStar++;
                        score += 1000;
                        PlaySoundEffect(L"Sounds/star.wav", 30);
                    }
                }
            } else {    // 3D
                if (isStage1 ? stage1[pz][py][px] == 'S' : stage2[pz][py][px] == 'S') {
                    isStage1 ? stage1[pz][py][px] = '.' : stage2[pz][py][px] = '>';
                    hotStar++;
                    score += 1000;
                    PlaySoundEffect(L"Sounds/star.wav", 30);
                }
            }
        }
    }

    // それ以外の衝突判定
    for (int i = 0; i < playerWidth; i++) {
        for (int j = 0; j < playerHeight; j++) {
            int px = (int)fPlayerX + i;
            int py = (int)fPlayerY;
            int pz = (int)fPlayerZ - j;

            if (is2DMode) { // 2D
                for (int y = 1; y <= 8; y++) {   // Y軸1~8まで描画しているからそこだけチェック
                    if (isStage1 ? stage1[pz][y][px] == 'c' : stage2[pz][y][px] == 'c') {  // コインとの衝突判定
                        isStage1 ? stage1[pz][y][px] = '.' : stage2[pz][y][px] = '>';
                        coinCount++;
                        score += 100;
                        PlaySoundEffect(L"Sounds/coin.wav", 30);
                    }
                    if (isStage1 ? stage1[pz][y][px] == 'F' || stage1[pz][y][px] == 'f' : stage2[pz][y][px] == 'F' || stage2[pz][y][px] == 'f') {  // バーナーとの衝突判定
                        currentScene = GameOverScene;
                        isGameOver = true;
                        return;
                    }
                }
            } else {    // 3D
                if (isStage1 ? stage1[pz][py][px] == 'c' : stage2[pz][py][px] == 'c') { //コインとの衝突判定
                    isStage1 ? stage1[pz][py][px] = '.' : stage2[pz][py][px] = '>';
                    coinCount++;
                    score += 100;
                    PlaySoundEffect(L"Sounds/coin.wav", 30);
                }
                if (isStage1 ? stage1[pz][py][px] == 'F' || stage1[pz][py][px] == 'f' : stage2[pz][py][px] == 'F' || stage2[pz][py][px] == 'f') {  // バーナーとの衝突判定
                    currentScene = GameOverScene;
                    isGameOver = true;
                    return;
                }
            }
        }
    }
#pragma endregion
}

// バーナーの点火と消火を切り替える
void ToggleBurner() {
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastBurnerToggle).count() >= 2) {
        isBurnerActive = !isBurnerActive;
        lastBurnerToggle = now;
    }
}

// ゴール条件のチェック
bool CheckGoalCondition() {
    if (hotStar >= 5) {
        for (int z = 0; z < nMapDepth; z++) {
            for (int y = 0; y < nMapHeight; y++) {
                for (int x = 0; x < nMapWidth; x++) {
                    if (stage1[z][y][x] == 'K') {
                        stage1[z][y][x] = '.';
                    }
                }
            }
        }
    }
    if (is2DMode) {
        for (int i = -1; i <= 1; i++) {
            for (int j = 0; j < playerWidth; j++) {
                for (int w = 0; w < 11; w++) {
                    if (isStage1 ? stage1[(int)fPlayerZ + i][w][(int)fPlayerX + j] == 'I' || stage1[(int)fPlayerZ + i][w][(int)fPlayerX + j] == 'Y' : stage2[(int)fPlayerZ + i][w][(int)fPlayerX + j] == 'I' || stage2[(int)fPlayerZ + i][w][(int)fPlayerX + j] == 'Y') {
                        return true;
                    }
                }
            }
        }
    }
    for (int i = -1; i <= 1; i++) {
        for (int j = 0; j < playerWidth; j++) {
            if (isStage1 ? stage1[(int)fPlayerZ + i][(int)fPlayerY][(int)fPlayerX + j] == 'I' || stage1[(int)fPlayerZ + i][(int)fPlayerY][(int)fPlayerX + j] == 'Y' : stage2[(int)fPlayerZ + i][(int)fPlayerY][(int)fPlayerX + j] == 'I' || stage2[(int)fPlayerZ + i][(int)fPlayerY][(int)fPlayerX + j] == 'Y') {
                return true;
            }
        }
    }
    return false;
}

// ゴール処理
void ProcessGoal(wchar_t* screen, WORD* screenColor, HANDLE hScreen) {
    static DWORD dwBytesWritten = 0;

    if (isGoalReached) {
        if (isPlayerMove) {
            PlaySoundEffect(L"Sounds/goal.wav", 50);
            StopBGM();
        }
        isPlayerMove = false;   // Playerの動きを止める
        gravity = 0;

        if (fPlayerZ < 20.0f) {     // ポールを下に下がる
            isMovingDown = true;
            fPlayerZ += 0.2f;
            if (fPlayerZ >= 20.0f) {
                fPlayerZ = 20.0f;
                isMovingDown = false;
            }
        } else if (!isMovingDown && fPlayerX < nMapWidth - 24) {    // 右に移動する
            isMovingRight = true;
            fPlayerX += (10.0f / 50.0f);
            if (fPlayerX >= nMapWidth - 24) {
                fPlayerX = nMapWidth - 24;
                isMovingRight = false;
            }
        } else if (fPlayerZ < 21.0f) {
            isMovingDown = true;
            fPlayerZ += 0.2f;
            if (fPlayerZ >= 21.0f) {
                fPlayerZ = 21.0f;
                isMovingDown = false;
            }
        } else if (!isMovingDown && fPlayerX < nMapWidth - 10) {
            isMovingRight = true;
            fPlayerX += (10.0f / 50.0f);
            if (fPlayerX >= nMapWidth - 10) {
                fPlayerX = nMapWidth - 10;
                isMovingRight = false;
            }
        } else {
            if (cachedTime > 0) {   // 残りTime分スコア加算
                std::this_thread::sleep_for(std::chrono::microseconds(1)); // 少し待つ
                if (cachedTime > 3) {
                    score += 300;
                    cachedTime = cachedTime - 3;
                } else {
                    score += 100;
                    cachedTime--;
                }
            } else {    // ResultScene移動
                std::this_thread::sleep_for(std::chrono::seconds(1)); // 1秒待つ
                currentScene = ResultScene;
            }
        }
    }
}

// リザルトシーンの描画
void DrawResultScene(wchar_t* screen, WORD* screenColor) {
    if (bgmCheak) {
        StopBGM();
        PlaySoundEffect(L"Sounds/congratulation.wav", 50);
        bgmCheak = false;
    }
    std::wstring result = L"Congratulations!";
    std::wstring instruction = L"Press Enter or A";
    int resultStart = (nScreenWidth - result.size()) / 2;
    int instructionStart = (nScreenWidth - instruction.size()) / 2;

    for (size_t i = 0; i < result.size(); i++) {
        screen[nScreenWidth * (nScreenHeight / 2 - 1) + resultStart + i] = result[i];
        screenColor[nScreenWidth * (nScreenHeight / 2 - 1) + resultStart + i] = FG_WHITE;
    }

    for (size_t i = 0; i < instruction.size(); i++) {
        screen[nScreenWidth * (nScreenHeight / 2 + 1) + instructionStart + i] = instruction[i];
        screenColor[nScreenWidth * (nScreenHeight / 2 + 1) + instructionStart + i] = FG_WHITE;
    }
}

// ゲームオーバーシーンの描画
void DrawGameOverScene(wchar_t* screen, WORD* screenColor) {
    if (bgmCheak) {
        StopBGM();
        PlaySoundEffect(L"Sounds/gameover.wav", 50);
        bgmCheak = false;
    }
    std::wstring gameOverText = L"GAME OVER";
    std::wstring instruction = L"Press Enter or A";

    int gameOverStart = (nScreenWidth - gameOverText.size()) / 2;
    int instructionStart = (nScreenWidth - instruction.size()) / 2;

    for (size_t i = 0; i < gameOverText.size(); i++) {
        screen[nScreenWidth * (nScreenHeight / 2 - 1) + gameOverStart + i] = gameOverText[i];
        screenColor[nScreenWidth * (nScreenHeight / 2 - 1) + gameOverStart + i] = FG_RED;
    }
    //int pos = 0;
    //for (size_t i = 0; i < instruction.size(); i++) {
    //    screen[nScreenWidth * (nScreenHeight / 2 + 1) + instructionStart + i + pos] = instruction[i];
    //    screenColor[nScreenWidth * (nScreenHeight / 2 + 1) + instructionStart + i + pos] = FG_RED;
    //    pos++;
    //    // 全角文字の場合、次の位置を1つ空ける
    //    if ((instruction[i] >= 0x4E00 && instruction[i] <= 0x9FFF) || // CJK統合漢字
    //        (instruction[i] >= 0x3000 && instruction[i] <= 0x303F) || // CJK記号および句読点
    //        (instruction[i] >= 0xFF00 && instruction[i] <= 0xFFEF))  // 半角および全角形
    //    {
    //        pos++;
    //    }
    //}

    for (size_t i = 0; i < instruction.size(); i++) {
        screen[nScreenWidth * (nScreenHeight / 2 + 1) + instructionStart + i] = instruction[i];
        screenColor[nScreenWidth * (nScreenHeight / 2 + 1) + instructionStart + i] = FG_WHITE;
    }
}

// 画面にスコア、コインの枚数、残り時間を表示する関数
void DrawScoreCoinAndTime(wchar_t* screen, WORD* screenColor) {
    if (!isGoalReached) {
        cachedTime = maxTimeLimit - std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count();
    }

    std::wstring scoreText = L"SCORE " + std::to_wstring(score);
    std::wstring coinText = L" COIN x " + std::to_wstring(coinCount);
    std::wstring timeText = L" Time " + std::to_wstring(cachedTime);

    // hotStarの表示
    std::wstring hotStarText = L"[";
    for (int i = 0; i < 5; i++) {
        if (i < hotStar) {
            hotStarText += 0x22C6; // 大きな星
        } else {
            hotStarText += L"."; // 空の星
        }
    }
    hotStarText += L"]";

    size_t index = 0;
    // スコアの描画
    for (size_t i = 0; i < scoreText.size(); i++, index++) {
        screen[index] = scoreText[i];
        screenColor[index] = isStage1 ? (FG_BLACK | BG_SKY) : (FG_WHITE | BG_BLACK);
    }

    // コイン数の描画
    for (size_t i = 0; i < coinText.size(); i++, index++) {
        screen[index] = coinText[i];
        screenColor[index] = isStage1 ? (FG_BLACK | BG_SKY) : (FG_WHITE | BG_BLACK);
    }

    // 時間の描画
    for (size_t i = 0; i < timeText.size(); i++, index++) {
        screen[index] = timeText[i];
        screenColor[index] = isStage1 ? (FG_BLACK | BG_SKY) : (FG_WHITE | BG_BLACK);
    }

    // hotStarの描画
    for (size_t i = 0; i < hotStarText.size(); i++, index++) {
        screen[index] = hotStarText[i];
        if (hotStarText[i] == 0x22C6) {
            screenColor[index] = isStage1 ? (FG_RED | BG_SKY) : (FG_RED | BG_BLACK);
        } else {
            screenColor[index] = isStage1 ? (FG_BLACK | BG_SKY) : (FG_WHITE | BG_BLACK);
        }
    }
}

// 時間制限をチェックする関数
void CheckTimeLimit() {
    if (!isGoalReached && std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count() >= maxTimeLimit) {
        currentScene = GameOverScene;
        isGameOver = true;
    }
}

// ステージを切り替える関数
void SwitchStage(wchar_t* screen, WORD* screenColor, HANDLE hScreen) {
    // クールダウン中は処理しない
    if (isPipeCoolingDown) {
        return;
    }

    PlaySoundEffect(L"Sounds/dokan.wav", 40);

    isSwitchStage = true;

    auto drawPlayerInPipe = [&]() {
        if (is2DMode) {
            Draw2D(screen, screenColor);
        } else {
            Draw3D(screen, screenColor);
        }

        // 画面の更新
        WriteConsoleOutputCharacterW(hScreen, screen, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);
        WriteConsoleOutputAttribute(hScreen, screenColor, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);
        };

    // 土管に入るアニメーション
    for (int i = 0; i < 3; i++) {
        fPlayerZ += 1;
        drawPlayerInPipe();

        // 0.2秒待つ
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 画面を下から上に緑色にするアニメーション
    for (int y = nScreenHeight - 1; y >= 0; y--) {
        for (int x = 0; x < nScreenWidth; x++) {
            screenColor[y * nScreenWidth + x] = FG_GREEN | BG_GREEN;
        }

        // 画面の更新
        WriteConsoleOutputAttribute(hScreen, screenColor, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);

        // 0.05秒待つ
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // ステージを切り替える
    isStage1 = !isStage1;

    // 画面を上から下に元の描画に戻すアニメーション
    for (int y = 0; y < nScreenHeight; y++) {
        // 元の描画を行う
        if (is2DMode) {
            Draw2D(screen, screenColor);
        } else {
            Draw3D(screen, screenColor);
        }

        // 緑色の部分を元の描画に戻す
        for (int row = y; row < nScreenHeight; row++) {
            for (int x = 0; x < nScreenWidth; x++) {
                screenColor[row * nScreenWidth + x] = FG_GREEN | BG_GREEN;
            }
        }

        // 画面の更新
        WriteConsoleOutputCharacterW(hScreen, screen, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);
        WriteConsoleOutputAttribute(hScreen, screenColor, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);

        // 0.05秒待つ
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    //StopSoundEffect();
    PlaySoundEffect(L"Sounds/dokan.wav", 60);

    // 土管から出るアニメーション
    for (int i = 0; i < 3; i++) {
        fPlayerZ -= 1;
        drawPlayerInPipe();

        // 0.2秒待つ
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    isPlayerMove = true;

    // クールダウンを開始
    isPipeCoolingDown = true;
    lastPipeEntry = std::chrono::steady_clock::now();

    // クールダウンが終わったらリセット
    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        isPipeCoolingDown = false;
        }).detach();
}

// *****************************************************************************
// main関数
// *****************************************************************************
int main() {
#pragma region StartUp
    int execLastTime;    // ゲーム処理をした時間（タイマー値）
    int fpsLastTime;     // デバッグ表示をした時間（タイマー値）
    int currentTime;     // 現在の時間（タイマー値）
    int frameCount;      // ゲームの処理をした回数

    bool changeMap = false;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleA("PARALLEL WORLD");     // タイトル設定
    DWORD mode = 0;

    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
    WORD* screenColor = new WORD[nScreenWidth * nScreenHeight];

    HANDLE hScreen = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hScreen);

    // コンソールカーソルを非表示にする
    HideConsoleCursor(hScreen);

    // 文字選択無効
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prev_mode;
    GetConsoleMode(hInput, &prev_mode);
    SetConsoleMode(hInput, prev_mode & ~ENABLE_QUICK_EDIT_MODE); // Quick Edit Modeを無効にする


    InitializeStageFromString(); // ステージの初期化
    InitPlayer();

    // main関数内の初期化部分に追加
    startTime = std::chrono::steady_clock::now();

    // タイマー分解能を高精度に設定
    timeBeginPeriod(1);

    // サウンドシステムの初期化
    InitSoundSystem();
#pragma endregion


    //// メモリ確保サイズ
    //const size_t memorySize = 4ULL * 1024 * 1024 * 1024;
    //std::vector<char> memory(memorySize);
    execLastTime = fpsLastTime = timeGetTime();    // 現在のタイマー値取得

    currentTime = frameCount = 0;

    lastBurnerToggle = std::chrono::steady_clock::now();  // バーナーのタイマー初期化

    while (1) {
        currentTime = timeGetTime();    // 現在のタイマー値

        // デバッグ用FPS表示 0.5秒に一回更新する
        if ((currentTime - fpsLastTime) >= DEBUG_TIMER) {
            g_fpsCounter = frameCount * 1000 / (currentTime - fpsLastTime);
            fpsLastTime = currentTime;    // 表示した時間を保存
            frameCount = 0;               // ゲームループカウントリセット
        }

        if ((currentTime - execLastTime) >= (1000 / 60)) {
            fElapsedTime = (currentTime - execLastTime) / 1000.0f;
            execLastTime = currentTime;
            frameCount++;

            if (currentScene == TutorialScene || currentScene == Stage1_1Scene || currentScene == Stage1_2Scene) {
                if (!isSwitchStage) {   // 土管遷移で移動量がイカレナイヨウニ
                    if (isPlayerMove == true) {
                        PlayerMovement(screen, screenColor, hScreen);   // Playerのキー入力
                    }
                } else {
                    isSwitchStage = false;
                }
                // ゴール処理
                ProcessGoal(screen, screenColor, hScreen);

            }

            ToggleBurner(); // バーナーの状態を切り替え

            // 画面のクリア
            for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
                screen[i] = ' ';
                screenColor[i] = FG_BLACK;
            }

#pragma region Scene
            // シーンに応じた描画
            switch (currentScene) {
            case TitleScene:
                PlayBGM(L"Sounds/BGM.wav", 50);
                DrawTitleScene(screen, screenColor);
                if (GetAsyncKeyState(VK_RETURN) & +0x8000 || inport(0x0203) == 2 || inport(0x0213) == 2 || inport(0x0223) == 2 || inport(0x0233) == 2) {
                    StopBGM();
                    currentScene = SelectScene;
                    Sleep(200);
                    InitializeStageFromString();
                    InitPlayer();
                }
                isPlayerMove = true;
                break;
            case SelectScene:
                DrawSelectScene(screen, screenColor);
                //changeMap = false;
                break;
            case Draw0Scene:
                PlaySoundEffect(L"Sounds/drawscene.wav", 50);
                Draw0(screen, screenColor);
                PlayBGM(L"Sounds/BGM.wav", 50);
                break;
            case Draw1Scene:
                PlaySoundEffect(L"Sounds/drawscene.wav", 50);
                Draw1(screen, screenColor);
                PlayBGM(L"Sounds/BGM.wav", 50);
                break;
            case Draw2Scene:
                PlaySoundEffect(L"Sounds/drawscene.wav", 50);
                Draw2(screen, screenColor);
                PlayBGM(L"Sounds/BGM.wav", 50);
                break;
            case TutorialScene:
                if (!firstCheak) {
                    isPlayerMove = true;
                    firstCheak = true;
                }
                if (is2DMode) {
                    Draw2D(screen, screenColor);
                } else {
                    Draw3D(screen, screenColor);
                }
                DrawScoreCoinAndTime(screen, screenColor); // スコア等を表示
                CheckTimeLimit(); // 時間制限をチェック
                break;
            case Stage1_1Scene:
                if (!firstCheak) {
                    isPlayerMove = true;
                    firstCheak = true;
                }
                if (is2DMode) {
                    Draw2D(screen, screenColor);
                } else {
                    Draw3D(screen, screenColor);
                }
                DrawScoreCoinAndTime(screen, screenColor); // スコア等を表示
                CheckTimeLimit(); // 時間制限をチェック
                break;
            case Stage1_2Scene:
                if (!firstCheak) {
                    isPlayerMove = true;
                    firstCheak = true;
                }
                if (is2DMode) {
                    Draw2D(screen, screenColor);
                } else {
                    Draw3D(screen, screenColor);
                }
                DrawScoreCoinAndTime(screen, screenColor); // スコア等を表示
                CheckTimeLimit(); // 時間制限をチェック
                break;
            case ResultScene:
                DrawResultScene(screen, screenColor);
                if (GetAsyncKeyState(VK_RETURN) & 0x8000 || inport(0x0203) == 2 || inport(0x0213) == 2 || inport(0x0223) == 2 || inport(0x0233) == 2) {
                    bgmCheak = true;
                    currentScene = SelectScene;
                    InitializeStageFromString();
                    InitPlayer();
                    Sleep(200);
                }
                break;
            case GameOverScene:
                DrawGameOverScene(screen, screenColor);
                // ゲームオーバーシーンの処理
                if (GetAsyncKeyState(VK_RETURN) & 0x8000 || inport(0x0203) == 2 || inport(0x0213) == 2 || inport(0x0223) == 2 || inport(0x0233) == 2) {
                    bgmCheak = true;
                    currentScene = SelectScene;
                    InitializeStageFromString();
                    InitPlayer();
                    Sleep(200);
                    isGameOver = false;
                }
                break;
            default:
                currentScene = TitleScene;
                break;
            }
#pragma endregion

            // デバッグ時に座標・FPSを表示
#ifdef _DEBUG
// 現在のコンソール属性を保存
            CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
            GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
            WORD saved_attributes = consoleInfo.wAttributes;

            // フォントカラーを白、バックグラウンドカラーを黒に設定
            WORD debugTextColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // 白
            WORD debugBackgroundColor = 0; // 黒

            // デバッグ情報の表示
            swprintf(screen, 40, L"X: %3.2f Y: %3.2f Z: %3.2f FPS: %d", fPlayerX, fPlayerY, fPlayerZ, g_fpsCounter);

            // デバッグ情報の位置を設定
            int debugTextPos = 0; // 画面左上に表示

            // デバッグ情報の色を設定
            for (int i = 0; i < 40; i++) {
                screenColor[debugTextPos + i] = debugTextColor | debugBackgroundColor;
            }

            // 画面の更新
            WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);
            WriteConsoleOutputAttribute(hConsole, screenColor, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);

            // デフォルトのコンソール属性に戻す
            SetConsoleTextAttribute(hConsole, saved_attributes);
#endif

            // 画面の更新
            screen[nScreenWidth * nScreenHeight - 1] = '\0';
            WriteConsoleOutputCharacterW(hScreen, screen, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);
            WriteConsoleOutputAttribute(hScreen, screenColor, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten);
        }
    }

    delete[] screen;
    delete[] screenColor;

    // サウンドシステムの終了
    //closesound(mp3Handle);
    ShutdownSoundSystem();

    //// 確保したメモリを解放
    //memory.clear();

    timeEndPeriod(1);
    return 0;
}