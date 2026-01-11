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
#include <random>
#include "mytools.h"

using namespace std;

void cmd_left_click_once(const string& params, bool ansi);
void cmd_left_click_loop(const string& params, bool ansi);
void cmd_left_click_loop_random(const string& params, bool ansi);
void cmd_exit(const string& params, bool ansi);
void cmd_lc_settings(const string& params, bool ansi);
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
    {"6 单次点击", cmd_left_click_once},
    {"6 固定间隔点击", cmd_left_click_loop},
    {"6 随机间隔点击", cmd_left_click_loop_random},
    {"4 列表长度", cmd_set_list_long},
    {"4 热键设置", cmd_set_hot_key},
    {"awa", cmd_exit},
    {"5 ---/...", cmd_lc_settings}, 
    {"5 ===/---", cmd_lc_settings}, 
    {"5 +++/---", cmd_lc_settings}, 
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
        cout << "执行：" << cmd << "          \r"; 
        ResetConsoleColor();
        Sleep(1000);
    #endif
    // 遍历命令表查找匹配命令
    int table_size = sizeof(command_table) / sizeof(CommandEntry);
    for (int i = 0; i < table_size; i++) {
        const auto& entry = command_table[i];
        // 只匹配以命令名开头的情况
        if (cmd.substr(0, entry.name.length()) == entry.name) {        	
            string params = fag(cmd,' ',1);
            // 去除参数前后空格
            //params.erase(0, params.find_first_not_of(" \t"));
            //params.erase(params.find_last_not_of(" \t") + 1);
			// DEBUG模式提示
    		#ifdef DEBUG
        		SetConsoleColor(BRIGHT_GREEN);
        		cout << "传入：" << params << "          \r"; 
        		ResetConsoleColor();
        		Sleep(1000);
    		#endif
            entry.func(params, ansi);
            
            // 命令执行完成后清除显示区
            clearDisplayArea(3,18);
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
    cout << "y=";
    cin >> cy;
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

void cmd_left_click_loop(const string& params, bool ansi) {
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
    int l;
    string cx = "",cy = "";
    double j;
    cout << "检查已通过!\n当前鼠标坐标可用~代替\n";
    bool al = 0;
    al = ask_only("应用到每一次循环？(Y/N)","Y,N",1,1)=="Y"?1:0;
	cout << "x=";
	cin >> cx;
	cout << "y=";
	cin >> cy;
    cout << "循环次数=";
    cin >> l;
    cout << "循环间隔(s)=";
    cin >> j;
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
    	int mx,my;
    	m_getxy(mx,my);
		int m1x = ssmath(cx,mx),m1y = ssmath(cy,my);
    	m_gotoxy(m1x,m1y);
        // 移动到目标位置并点击
        for(int i = 0;i < l;i++){
        	mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        	Sleep(j*1000);
        	printf("#");
        	cout << i+1 << "\r";
        	if(al){
    			m_getxy(mx,my);
    			m1x = ssmath(cx,mx),m1y = ssmath(cy,my);
        		m_gotoxy(m1x,m1y);
			}
		}
		m_gotoxy(mx,my);
        cout << "执行成功，请查看是否有效！\n";
    }

    system("pause");
    return;
}

void cmd_left_click_loop_random(const string& params, bool ansi) {
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
    int l;
    string cx = "",cy = "";
    double j1,j2;
    cout << "检查已通过!\n当前鼠标坐标可用~代替\n";
    bool al = 0;
    al = ask_only("应用到每一次循环？(Y/N)","Y,N",1,1)=="Y"?1:0;
	cout << "x=";
	cin >> cx;
	cout << "y=";
	cin >> cy;
    cout << "循环次数=";
    cin >> l;
    cout << "循环最小间隔(s)=";
    cin >> j1;
    cout << "循环最大间隔(s)=";
    cin >> j2;
    // 初始化状态变量
    hotkeyTriggered = false;
    hotkeyCancelled = false;
    
    // 安装键盘钩子
    installHook();
    if (!hKeyboardHook) {
        r_show_error("无法安装热键钩子！", ansi);
        return;
    }
    cout << "请稍等，正在生成随机数……";
	
    int ran_[l] = {0};
    
    // 步骤 1：创建随机数引擎（mt19937 推荐首选）
    mt19937 engine;
    
    // 步骤 2：设置随机数种子（避免固定序列）
    random_device rd;
    
    // 步骤 3：创建整数均匀分布，指定范围 [j1, j2]（闭区间，包含 j1 和 j2）
    uniform_int_distribution<int> int_dist(j1*1000, j2*1000);
    
    long long seed;
    for(int i = 0;i < l;i++){
    	try {
    	    // 优先使用 random_device 生成真随机种子
    	    engine.seed(rd());
    	} catch (...) {
    	    // 备用方案：系统时间戳作为种子（兼容不支持 random_device 的系统）
    	    seed = chrono::system_clock::now().time_since_epoch().count();
    	    engine.seed(seed);
    	}
    	// 调用分布对象，传入引擎，生成符合要求的随机数
        ran_[i] = int_dist(engine);
	}
    
    cout << "\r等待热键触发... (F1执行, ESC取消)\n";
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
    	int mx,my;
    	m_getxy(mx,my);
		int m1x = ssmath(cx,mx),m1y = ssmath(cy,my);
    	m_gotoxy(m1x,m1y);
        // 移动到目标位置并点击
        for(int i = 0;i < l;i++){
        	mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        	mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        	Sleep(ran_[i]);
        	printf("#");
        	cout << i+1 << " - " << ran_[i] << "ms\r";
        	if(al){
    			m_getxy(mx,my);
    			m1x = ssmath(cx,mx),m1y = ssmath(cy,my);
        		m_gotoxy(m1x,m1y);
			}
		}
		m_gotoxy(mx,my);
        cout << "执行成功，请查看是否有效！\n";
    }

    system("pause");
    return;
}

void cmd_exit(const string& params, bool ansi) {
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
void cmd_lc_settings(const string& params, bool ansi){
	cout << params << "\n";
	change_data("line_consoles",params);
    lc_y = fag(params,'/',0);
    lc_n = fag(params,'/',1);
    cout << data[find_title("line_consoles",total_lines,data)][4];
	system("pause");
}

#endif // RUNNER_H
