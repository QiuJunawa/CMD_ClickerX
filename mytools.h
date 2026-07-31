#ifndef MYTOOLS_H
#define MYTOOLS_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include <conio.h>

using namespace std;

// ===================== 颜色常量 =====================
enum ConsoleColors {
    BLACK = 0,
    BLUE = FOREGROUND_BLUE,
    GREEN = FOREGROUND_GREEN,
    CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE,
    RED = FOREGROUND_RED,
    MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE,
    YELLOW = FOREGROUND_RED | FOREGROUND_GREEN,
    WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    GRAY = FOREGROUND_INTENSITY,
    BRIGHT_BLUE = FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    BRIGHT_GREEN = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    BRIGHT_CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    BRIGHT_RED = FOREGROUND_RED | FOREGROUND_INTENSITY,
    BRIGHT_MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    BRIGHT_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    BRIGHT_WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
};

// ===================== ANSI 支持 =====================
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// 函数声明（只声明，不实现）
bool supportsAnsiEscapes();
void enableAnsiOnWindows();

// ===================== 控制台操作 =====================
void gotoxy(int x, int y);
COORD getxy();
void SetConsoleColor(WORD color);
void ResetConsoleColor();
void SetConsoleCursorVisible(bool visible);
void CleanLine(int start, int end);

// ===================== 工具函数 =====================
int set0(int tmp);
int set_max(int tmp, int max_val);
int set_min(int tmp, int min_val);
std::string find_data(std::string node, int px, int total, std::string data[][10]);
int find_title(const std::string& title, int total, const std::string data[][10]);

// ===================== 安全防护 =====================
// 计算 HMAC-SHA256，返回十六进制字符串
std::string calculate_hmac(const std::string& password, const std::string& key);
#include <windows.h>
#include <string>

std::string getMachineGuid();

#endif // MYTOOLS_H