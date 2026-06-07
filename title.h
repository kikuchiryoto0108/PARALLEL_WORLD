#pragma once

#include "common.h"
void DrawTitleScene(wchar_t* screen, WORD* screenColor);

// タイトルシーンの描画
void DrawTitleScene(wchar_t* screen, WORD* screenColor) {
    std::wstring title[] = {
L"................................................................................",
L"....00000000.00000000.00000000.00000000.00.......00.......00000000.00...........",
L"...0GGGGGGGG0YYYYYYYY0BBBBBBBB0PPPPPPPP0YY0.....0GG0.....0RRRRRRRR0BB0..........",
L"...0GG0000GG0YY0000YY0BB0000BB0PP0000PP0YY0.....0GG0.....0RR0000000BB0..........",
L"...0GG0000GG0YY0..0YY0BB0000BB0PP0..0PP0YY0.....0GG0.....0RR0.....0BB0..........",
L"...0GGGGGGGG0YY0000YY0BBBBBBBB0PP0000PP0YY0.....0GG0.....0RR0000000BB0..........",
L"...0GG0000000YYYYYYYY0BB000BB00PPPPPPPP0YY0.....0GG0.....0RRRRRRRR0BB0..........",
L"...0GG0.....0YY0000YY0BB0.0BB00PP0000PP0YY0.....0GG0.....0RR0000000BB0..........",
L"...0GG0.....0YY0..0YY0BB0..0BB0PP0..0PP0YY0000000GG0000000RR0000000BB0..........",
L"...0GG0.....0YY0..0YY0BB0..0BB0PP0..0PP0YYYYYYYY0GGGGGGGG0RRRRRRRR0BBBBBBBB0....",
L"....00.......00...000.0000.00000000.000000000000.00000000000000000.00000000.....",
L".................0RR0..0RR0YYYYYYYY0PPPPPPPP0GG0.....0BBBBBB0...................",
L".................0RR0..0RR0YY0000YY0PP0000PP0GG0.....0BB00BBB0..................",
L".................0RR0..0RR0YY0..0YY0PP0000PP0GG0.....0BB0.00BB0.................",
L".................0RR0000RR0YY0..0YY0PPPPPPPP0GG0.....0BB0..0BB0.................",
L".................0RR0RR0RR0YY0..0YY0PP000PP00GG0.....0BB0..0BB0.................",
L".................0RR0RR0RR0YY0..0YY0PP0.0PP00GG0.....0BB0.00BB0.................",
L".................0RR0RR0RR0YY0000YY0PP0..0PP0GG0000000BB00BBB0..................",
L".................0RRRRRRRR0YYYYYYYY0PP0..0PP0GGGGGGGG0BBBBBB0...................",
L"..................00000000.00000000.00....00.00000000.0000000...................",
L"................................................................................",
L"..................................press enter...................................",
L"................................................................................",
L"................................................................................",
L"................................................................................",
    };

    for (size_t i = 0; i < 25; i++) {
        for (size_t j = 0; j < 80; j++) {
            screen[i * 80 + j] = title[i][j];
            switch (title[i][j]) {
            case L'.':
                screenColor[i * 80 + j] = FG_WHITE | BG_WHITE;
                break;
            case L'Y':
                screenColor[i * 80 + j] = FG_DARK_YELLOW | BG_DARK_YELLOW;
                break;
            case L'B':
                screenColor[i * 80 + j] = FG_BLUE | BG_BLUE;
                break;
            case L'P':
                screenColor[i * 80 + j] = FG_PURPLE | BG_PURPLE;
                break;
            case L'R':
                screenColor[i * 80 + j] = FG_RED | BG_RED;
                break;
            case L'G':
                screenColor[i * 80 + j] = FG_GREEN | BG_GREEN;
                break;
            case L'0':
                screenColor[i * 80 + j] = FG_BLACK | BG_BLACK;
                break;


            case L'p':
                screenColor[i * 80 + j] = FG_BLACK | BG_WHITE;
                break;
            case L'r':
                screenColor[i * 80 + j] = FG_BLACK | BG_WHITE;
                break;
            case L'e':
                screenColor[i * 80 + j] = FG_BLACK | BG_WHITE;
                break;
            case L's':
                screenColor[i * 80 + j] = FG_BLACK | BG_WHITE;
                break;
            case L'n':
                screenColor[i * 80 + j] = FG_BLACK | BG_WHITE;
                break;
            case L't':
                screenColor[i * 80 + j] = FG_BLACK | BG_WHITE;
                break;
            case L' ':
                screenColor[i * 80 + j] = FG_BLACK | BG_WHITE;
                break;
            default:
                screenColor[i * 80 + j] = FG_BLACK | BG_BLACK;
                break;
            }
        }
    }
}