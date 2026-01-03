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

// 如果 ENABLE_VIRTUAL_TERMINAL_PROCESSING 未定义，手动定义它
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
//启用debug模式 
#define _DEBUG





string setstr(int num) {
    if (num == 0) {
        return "0";
    }
    
    std::string ret;
    bool negative = false;
    
    // 处理负数
    if (num < 0) {
        negative = true;
        num = -num;
    }
    
    // 逐个提取数字
    while (num > 0) {
        ret.push_back('0' + (num % 10));
        num /= 10;
    }
    
    // 反转字符串得到正确顺序
    std::reverse(ret.begin(), ret.end());
    
    // 添加负号
    if (negative) {
        ret.insert(ret.begin(), '-');
    }
    
    return ret;
}
// 负责将字符串“十进制数字”转为数字（如果可以）
int setnum(string num) {
    // 先处理空字符串
    if (num.empty()) {
        return -1;
    }
    
    int rnum = 0;
    // 正向遍历：从字符串开头（最高位）到末尾（最低位）
    for (int i = 0; i < num.length(); i++) {
        // 校验是否为数字字符
        if (num[i] < '0' || num[i] > '9') {
            return -1; // 直接返回-1，无需继续遍历
        }
        // 正确的十进制数字计算逻辑
        rnum = rnum * 10 + (num[i] - '0');
    }
    
    // 可选：添加数值范围校验（避免超出unsigned short范围）
    if (rnum < 0 || rnum > USHRT_MAX) {
        return -1;
    }
    
    return rnum;
}
// 检测当前控制台是否支持 ANSI 转义码
extern bool supportsAnsiEscapes() {
    // 获取标准输出句柄
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // 检查是否为控制台
    DWORD mode;
    if (!GetConsoleMode(hConsole, &mode)) {
        return false; // 不是控制台
    }
    
    // 检查是否启用了虚拟终端处理
    return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}

// 在 Windows 上启用 ANSI 转义码支持
extern void enableAnsiOnWindows() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hConsole, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, mode);
    }
}

// 3个越界重置函数 
extern int set0(int tmp) {
    if (tmp < 0) {
        return 0;
    }
    return tmp;
}

extern int set_max(int tmp, int max_val) {
    if (tmp > max_val) {
        return max_val;
    }
    return tmp;
}

extern int set_min(int tmp, int min_val) {
    if (tmp < min_val) {
        return min_val;
    }
    return tmp;
}

// 数target出现了几次 
extern int countchar(const std::string& str, char target) {
    return std::count(str.begin(), str.end(), target);
}

// 分割字符串（支持连续分隔符，保留空元素）
extern std::string fag(const std::string& str, char delimiter, size_t index) {
    std::vector<std::string> tokens;
    std::string token;
    size_t pos = 0;
    size_t last_pos = 0;
    
    while (pos <= str.length()) {
        pos = str.find(delimiter, last_pos);
        token = str.substr(last_pos, pos - last_pos);
        tokens.push_back(token);
        last_pos = pos + 1;
    }
    
    // 检查索引是否有效
    if (index < tokens.size()) {
        return tokens[index];
    }
    
    return ""; // 索引超出范围返回空字符串
}

// 移动光标 
extern void gotoxy(int x, int y) {
    // 获取标准输出句柄
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // 设置光标位置
    COORD coord;
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    
    SetConsoleCursorPosition(hConsole, coord);
}

// 获取光标位置 
extern COORD getxy() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        return csbi.dwCursorPosition;
    }
    
    // 如果获取失败，返回(0,0)
    COORD invalid = {0, 0};
    return invalid;
}

// 设置控制台文本颜色
extern void SetConsoleColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

// 恢复默认颜色 (灰字黑底)
extern void ResetConsoleColor() {
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

// 隐藏/显示光标
extern void SetConsoleCursorVisible(bool visible) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hConsole, &cci);
    cci.bVisible = visible;
    SetConsoleCursorInfo(hConsole, &cci);
}

// 颜色常量定义
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

extern string find_data(string node, int px, int total, string data[][10]) {
    // 遍历所有行
    for (int i = 0; i < total; i++) {
        if (data[i][1] == node) {
            // 找到匹配的行，处理data[i][4]
            string list = data[i][4];
            vector<string> items;
            stringstream ss(list);
            string item;
            
            // 按逗号分割字符串
            while (getline(ss, item, ',')) {
                items.push_back(item);
            }
            
            // 检查px是否有效
            if (px >= 0 && px < (int)items.size()) {
                return items[px];
            } else {
                return ""; // 无效的px索引
            }
        }
    }
    
    return ""; // 未找到匹配的node
} 

/**
 * 查找标题所在的行号
 * @param title 要查找的标题（完全匹配data[i][1]）
 * @param total data数组的有效行数
 * @param data 二维字符串数组
 * @return 找到返回行号（≥0），未找到返回-1
 */
extern int find_title(const string& title, int total, const string data[][10]) {
    for (int i = 0; i < total; i++) {
        if (data[i][1] == title) {
            return i; // 返回匹配的行号
        }
    }
    return -1; // 未找到
}

// 新增：检测鼠标滚轮事件（非阻塞）
extern bool checkMouseWheel(int& delta) {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD numEvents = 0;
    if (!GetNumberOfConsoleInputEvents(hInput, &numEvents) || numEvents == 0) {
        return false;
    }

    INPUT_RECORD inputRecords[128];
    DWORD numRead = 0;
    if (!ReadConsoleInput(hInput, inputRecords, 128, &numRead)) {
        return false;
    }

    bool wheelDetected = false;
    delta = 0;

    for (DWORD i = 0; i < numRead; ++i) {
        const INPUT_RECORD& record = inputRecords[i];
        if (record.EventType == MOUSE_EVENT) {
            const MOUSE_EVENT_RECORD& mouseEvent = record.Event.MouseEvent;
            if (mouseEvent.dwEventFlags == MOUSE_WHEELED) {
                // 获取滚轮滚动值（正数向上，负数向下）
                delta = GET_WHEEL_DELTA_WPARAM(mouseEvent.dwButtonState);
                wheelDetected = true;
            }
        } else {
            // 将非鼠标事件放回输入缓冲区
            WriteConsoleInput(hInput, &record, 1, &numRead);
        }
    }

    return wheelDetected;
}

// 新增：设置控制台输入模式（启用/禁用鼠标）
extern void setConsoleInputMode(bool enableMouse) {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hInput, &mode)) {
        if (enableMouse) {
            // 启用鼠标输入 + 扩展模式 + 禁用快速编辑（避免干扰）
            mode |= ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE;
        } else {
            // 恢复默认（禁用鼠标输入）
            mode &= ~ENABLE_MOUSE_INPUT;
        }
        SetConsoleMode(hInput, mode);
    }
}
// 新增 paint_list 函数
void paint_list(int x, int y, int nowline, int cc, string text[], int long__, bool disable_ansi, string& huanchong) {
    huanchong = "";
    gotoxy(x, y);

    // 短列表显示逻辑
    if (cc <= 2 * long__ + 1 || long__ < 0) {
        // 显示标题
        if (!disable_ansi) {
            cout << "\033[37m" << text[0] << "\033[K\033[0m" << endl;
            cout << "\033[90m---\033[K\033[0m" << endl;
        } else {
            SetConsoleColor(WHITE);
            cout << text[0] << string(80 - text[0].length(), ' ') << endl;
            SetConsoleColor(GRAY);
            cout << "---" << string(77, ' ') << endl;
            ResetConsoleColor();
        }

        // 显示选项
        for (int i = 0; i < cc; i++) {
            if (i == nowline) {
                if (!disable_ansi) {
                    huanchong += "\033[32m-> " + text[i+1] + "\033[K\033[0m\n";
                } else {
                    huanchong += "-> " + text[i+1] + string(80 - text[i+1].length() - 3, ' ') + "\n";
                }
            } else {
                if (!disable_ansi) {
                    huanchong += "- " + text[i+1] + "\033[K\n";
                } else {
                    huanchong += "- " + text[i+1] + string(80 - text[i+1].length() - 2, ' ') + "\n";
                }
            }
        }

        // 显示底部分隔线
        if (!disable_ansi) {
            cout << huanchong;
            cout << "\033[90m---\033[K\033[0m";
        } else {
            cout << huanchong;
            SetConsoleColor(GRAY);
            cout << "---" << string(77, ' ');
            ResetConsoleColor();
        }
    } 
    // 长列表显示逻辑
    else {
        // 显示标题
        if (!disable_ansi) {
            cout << "\033[37m" << text[0] << "\033[K\033[0m" << endl;
        } else {
            SetConsoleColor(WHITE);
            cout << text[0] << string(80 - text[0].length(), ' ') << endl;
            ResetConsoleColor();
        }

        // 显示顶部省略/分隔线
        if (!disable_ansi) {
            cout << "\033[90m";
        } else {
            SetConsoleColor(GRAY);
        }
        if (nowline > long__) {
            cout << "...";
        } else {
            cout << "---";
        }
        cout << string(77, ' ');
        if (!disable_ansi) {
            cout << "\033[0m" << endl;
        } else {
            cout << endl;
            ResetConsoleColor();
        }

        // 计算可视范围
        int start_idx = 0;
        if (nowline > long__) {
            start_idx = nowline - long__;
            if (start_idx + 2 * long__ + 1 > cc - 1) {
                start_idx = cc - 1 - 2 * long__;
            }
        }
        start_idx = set0(start_idx);

        // 显示可视选项
        for (int i = start_idx; i < start_idx + 2 * long__ + 1 && i < cc; i++) {
            if (i == nowline) {
                if (!disable_ansi) {
                    huanchong += "\033[32m-> " + text[i+1] + "\033[K\033[0m\n";
                } else {
                    huanchong += "-> " + text[i+1] + string(80 - text[i+1].length() - 3, ' ') + "\n";
                }
            } else {
                if (!disable_ansi) {
                    huanchong += "- " + text[i+1] + "\033[K\n";
                } else {
                    huanchong += "- " + text[i+1] + string(80 - text[i+1].length() - 2, ' ') + "\n";
                }
            }
        }

        // 显示选项
        cout << huanchong;

        // 显示底部省略/分隔线
        if (!disable_ansi) {
            cout << "\033[90m";
        } else {
            SetConsoleColor(GRAY);
        }
        if (nowline + long__ < cc - 1) {
            cout << "...";
        } else {
            cout << "---";
        }
        cout << string(77, ' ');
        if (!disable_ansi) {
            cout << "\033[0m";
        } else {
            ResetConsoleColor();
        }
    }
    
}
// 核心：解析控制台鼠标事件，提取滚轮信息
SHORT ProcessConsoleMouseEvent(const MOUSE_EVENT_RECORD& mouseEvent)
{
    if (mouseEvent.dwEventFlags == MOUSE_WHEELED)
    {
        SHORT wheelDelta = GET_WHEEL_DELTA_WPARAM(mouseEvent.dwButtonState);
        return wheelDelta;
    }
    return 0;
}
// 辅助函数：恢复控制台默认输入模式（启用回显）
void restore_console_echo_mode() {
/*
#ifndef _WIN32
    return;
#endif

    HANDLE hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hConsoleInput == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD dwOldMode;
    if (GetConsoleMode(hConsoleInput, &dwOldMode)) {
        // 恢复回显和行输入（仅补全缺失的标志，不重复设置）
        DWORD dwNewMode = dwOldMode | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT;
        SetConsoleMode(hConsoleInput, dwNewMode);
    }
    //*/
}
int set_inspace(int num,int min,int max){
	if(num > max){
		num = num % min;
		if(num < min)
			num = min + 1;
	}
	else if(num < min){
		num = (num + min) % max + min;
		if(num > max)
			num = max - 1;
	}
	return num;
}

#endif // MYTOOLS_H
