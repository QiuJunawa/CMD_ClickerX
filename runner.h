#ifndef RUNNER_H
#define RUNNER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include <conio.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include "mytools.h"

using namespace std;

void cmd_left_click_once(const string& params, bool ansi);
void cmd_exit(const string& params, bool ansi);
void cmd_settings(const string& params, bool ansi);
void cmd_set_list_long(const string& params, bool ansi);
void cmd_set_hot_key(const string& params, bool ansi);


int show_long = 100;
string data[10000][10];
int total_lines = 0; // 全局有效行数，避免重复读取

// 全局变量用于键盘钩子
HHOOK hKeyboardHook = NULL;
atomic<bool> hotkeyTriggered(false);
atomic<bool> hotkeyCancelled(false);

// 命令函数指针类型
typedef void (*CommandFunc)(const string&, bool);

// 命令映射表
struct CommandEntry {
    string name;
    CommandFunc func;
} command_table[] = {
    //{"左键连点", cmd_left_click},
    {"退出程序", cmd_exit},
    {"设置", cmd_settings},
    {"美化选择器线条", cmd_settings},
    {"6 单次点击", cmd_left_click_once},
    {"4 列表长度", cmd_set_list_long},
    {"4 热键设置", cmd_set_hot_key},
    {"awa", cmd_exit},
    {"到底啦/我还有", cmd_settings}
};

void change_data(string index, string data_cd) {
    // 遍历数据查找匹配的第一列名称
    int target_row = -1;
    for (int i = 0; i < total_lines; ++i) {
        if (data[i][1] == index) {  // 匹配第一列（名称列）
            target_row = i;
            break;
        }
    }

    // 检查是否找到对应行
    if (target_row == -1) {
        cerr << "错误：未找到名称为 '" << index << "' 的数据行" << endl;
        return;
    }

    // 更新第四列数据（[][3]）
    data[target_row][4] = data_cd;
    //cout << "已更新名称为 '" << index << "' 的第四列数据为：" << data_cd << endl;
}

void r_show_error(const string& msg, bool ansi) {
    if (!ansi) {
        cerr << "\033[91m"; // 亮红色
    } else {
        SetConsoleColor(BRIGHT_RED);
    }
    cerr << msg << endl;
    if (!ansi) {
        cerr << "\033[0m";
    } else {
        ResetConsoleColor();
    }
}

unsigned short hooks = VK_F1;
// 键盘钩子回调函数
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) { // 仅处理按键按下事件
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
        if (pKeyBoard->vkCode == hooks) {
            hotkeyTriggered = true;
            return 1;
        } else if (pKeyBoard->vkCode == VK_ESCAPE) {
            hotkeyCancelled = true;
            return 1;
        }
        // 其他按键直接放行，不拦截
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

// 安装键盘钩子
void installHook() {
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
}

// 卸载键盘钩子
void uninstallHook() {
    if (hKeyboardHook) {
        UnhookWindowsHookEx(hKeyboardHook);
        hKeyboardHook = NULL;
    }
}
// 在mytools.h中添加清除显示区的函数声明（放在extern void clearDisplayArea();

// 在mytools.cpp或合适的实现文件中添加函数定义
void clearDisplayArea(int y1,int y2) {
    int zx = getxy().X,zy = getxy().Y;
    gotoxy(0,y1);
    while(getxy().Y <= y2){
    	cout << "                                                            \n";
	}
	return;
}
// 修改execute_command函数，在命令执行后调用清除函数
void execute_command(const string& cmd, bool ansi) {
	// DEBUG模式提示
    #ifdef DEBUG
        SetConsoleColor(BRIGHT_GREEN);
        cout << "执行：" << cmd; 
        ResetConsoleColor();
        Sleep(1000);
    #endif
    // 遍历命令表查找匹配命令
    int table_size = sizeof(command_table) / sizeof(CommandEntry);
    for (int i = 0; i < table_size; i++) {
        const auto& entry = command_table[i];
        // 只匹配以命令名开头的情况
        if (cmd.substr(0, entry.name.length()) == entry.name) {        	
            string params = cmd.substr(entry.name.length());
            // 去除参数前后空格
            params.erase(0, params.find_first_not_of(" \t"));
            params.erase(params.find_last_not_of(" \t") + 1);
            entry.func(params, ansi);
            
            // 命令执行完成后清除显示区
            clearDisplayArea(3,15);
            return;
        }
    }

    // 没有找到匹配的命令
    // 未知命令处理后也清除显示区
    if (ansi) SetConsoleColor(BRIGHT_RED);
    cerr << "未知命令: " << cmd << endl;
    if (ansi) ResetConsoleColor();
    Sleep(1000);
    clearDisplayArea(3,10);
    
}

// 具体命令实现
void cmd_left_click_once(const string& params, bool ansi) {
	restore_console_echo_mode();
    // 卸载钩子
    uninstallHook();
    if (ansi) SetConsoleColor(BRIGHT_GREEN);
    cout << "执行左键连点功能" << (params.empty() ? "" : "，参数：" + params) << endl;
    if (ansi) ResetConsoleColor();
    cout << "正在检查系统环境……\n";
    #ifndef _WIN32
        cerr << "错误：本程序仅支持Windows系统，不支持当前操作系统！" << endl;
        return; // 非Windows系统返回
    #endif
    int cx,cy;
    cout << "检查已通过!\n执行左键点击的位置：\nx=";
    cin >> cx;
    cout << "\ny=";
    cin >> cy;
    cout << "\n";
    // 初始化状态变量
    hotkeyTriggered = false;
    hotkeyCancelled = false;
    
    // 安装键盘钩子
    installHook();
    if (!hKeyboardHook) {
        r_show_error("无法安装热键钩子！", ansi);
        return;
    }

    cout << "等待热键触发... (F1执行, ESC取消)\n";
    
    // 消息循环，确保钩子能正常工作
    MSG msg;
    while (!hotkeyTriggered && !hotkeyCancelled) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        this_thread::sleep_for(chrono::milliseconds(10)); // 减少CPU占用
    }

    // 卸载钩子
    uninstallHook();

    if (hotkeyCancelled) {
        cout << "已取消操作\n";
    }

    if (hotkeyTriggered) {
        // 移动到目标位置并点击
        SetCursorPos(cx, cy);
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        cout << "执行成功，请查看是否有效！\n";
    }

    system("pause");
    return;
}

void cmd_exit(const string& params, bool ansi) {
    if (ansi) SetConsoleColor(BRIGHT_YELLOW);
    cout << "准备退出程序..." << endl;
    if (ansi) ResetConsoleColor();
    Sleep(1000);
}

void cmd_settings(const string& params, bool ansi) {
    if (ansi) SetConsoleColor(BRIGHT_BLUE);
    cout << "进入设置菜单" << (params.empty() ? "" : "，参数：" + params) << endl;
    if (ansi) ResetConsoleColor();
    
    // 这里可以添加设置菜单的初始化代码
    // ...
    
    Sleep(1000);
}
void cmd_set_list_long(const string& params, bool ansi) {
	cout << "输入想要的列表长度："; 
	cin >> show_long;
	change_data("这是数据存储-showlong",setstr(show_long));
	cout << "设置成功！";
	return;
}
void cmd_set_hot_key(const string& params, bool ansi){
	cout << "未开放功能!";
	return;
	int a = getch();
	if(a == 0||a == 224){
		a = getch();
		change_data("更改热键",setstr(a));
	}else
	{
		change_data("更改热键",setstr(a));		
	}
}

#endif // RUNNER_H
