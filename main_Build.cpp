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
#include "runner.h"

#define A_VERSION 5
#define B_VERSION 2
#define C_VERSION 0

using namespace std;

// 全局变量存储数据
/* data[x][1]：名字
          [2,3]：x，y坐标
          [4]：第一个是标题，后面是选项 */ 
string cover[10001], ftl[10001];
int chooser_row = 0;

string setstr(int num); 
std::string restring(std::string s) {
    std::reverse(s.begin(), s.end());
    return s;
}

int cne(const std::string ftl[], int size) {
    return std::count_if(ftl, ftl + size, 
                        [](const std::string& s) { return !s.empty(); });
}



// 补全add_data函数
bool add_data(int lines, string name, int d_x, int d_y, string data_content) {
    if (lines < 0 || lines >= 10000) { // 检查数组边界
        return false;
    }
    data[lines][1] = name;
    data[lines][2] = setstr(d_x);
    data[lines][3] = setstr(d_y);
    data[lines][4] = data_content;
    // 清空后续列
    for (int j = 5; j < 10; j++) {
        data[lines][j] = "";
    }
    return true;
}

// 进度条函数
class Loader {
private:
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::mutex mtx_;
    std::condition_variable cv_;
    int progress_{0};
    int width_{50};
    
    void worker_func() {
        while (running_) {
            // 显示进度条
            std::cout << "\r[";
            int pos = progress_ * width_ / 100;
            for (int i = 0; i < width_; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << "|";
                else std::cout << "-";
            }
            std::cout << "] " << progress_ << "%";
            std::cout.flush();
            
            // 等待更新或停止
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait_for(lock, std::chrono::milliseconds(100), [this]{
                return !running_;
            });
        }
    }

public:
    // 开始进度条显示
    void start() {
        if (!running_) {
            running_ = true;
            worker_ = std::thread(&Loader::worker_func, this);
        }
    }

    // 更新进度 (0-100)
    void update(int progress) {
        std::lock_guard<std::mutex> lock(mtx_);
        progress_ = set_min(set_max(progress, 0), 100); // 限制在0-100
        cv_.notify_one();
    }

    // 停止并清理进度条
    void stop() {
        if (running_) {
            {
                std::lock_guard<std::mutex> lock(mtx_);
                running_ = false;
                cv_.notify_one();
            }
            if (worker_.joinable()) {
                worker_.join();
            }
            std::cout << std::endl;  // 换行
        }
    }

    ~Loader() {
        stop();
    }
};

// 读取 settings.dll 文件并返回读取的行数
int readSettings() {
    string file_path = "settings.dll";
    ifstream file(file_path);
    if (!file.is_open()) {
        cerr << "无法打开 " << file_path << " 文件，将创建新文件" << endl;
        return 0;
    }

    string line;
    int lines_read = 0;
    
    while (getline(file, line) && lines_read < 10000) {
        // 清空当前行的data数据
        for (int j = 1; j < 10; j++) {
            data[lines_read][j] = "";
        }
        
        // 去除行首尾空格
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) { // 跳过空行
            continue;
        }
        
        // 解析前3个字段：菜单名(data[1])、X坐标(data[2])、Y坐标(data[3])
        size_t pos1 = line.find(' ');
        if (pos1 == string::npos) { // 无空格，仅菜单名
            data[lines_read][1] = line;
            lines_read++;
            continue;
        }
        data[lines_read][1] = line.substr(0, pos1); // 菜单名
        
        size_t pos2 = line.find(' ', pos1 + 1);
        if (pos2 == string::npos) { // 只有2个字段
            data[lines_read][2] = line.substr(pos1 + 1);
            lines_read++;
            continue;
        }
        data[lines_read][2] = line.substr(pos1 + 1, pos2 - pos1 - 1); // X坐标
        
        size_t pos3 = line.find(' ', pos2 + 1);
        if (pos3 == string::npos) { // 只有3个字段
            data[lines_read][3] = line.substr(pos2 + 1);
            lines_read++;
            continue;
        }
        data[lines_read][3] = line.substr(pos2 + 1, pos3 - pos2 - 1); // Y坐标
        
        // 剩余内容作为菜单内容(data[4])
        data[lines_read][4] = line.substr(pos3 + 1);
        
        lines_read++;
    }
    
    file.close();
    return lines_read;
}

// 保存 settings.dll 文件
void saveSettings(int total_lines) {
    string file_path = "settings.dll";
    ofstream file(file_path);
    if (!file.is_open()) {
        cerr << "无法创建 " << file_path << " 文件，请检查权限" << endl;
        return;
    }

    for (int i = 0; i < total_lines; i++) {
        bool first_output = true;
        for (int j = 1; j < 10; j++) {
            if (!data[i][j].empty()) {
                if (!first_output) {
                    file << " ";
                }
                file << data[i][j];
                first_output = false;
            }
        }
        file << endl;
    }
    
    file.close();
}

// 辅助函数
bool confirm_exit(bool ansi) {
    if (!ansi) {
        cout << "\033[93m"; // 亮黄色
    } else {
        SetConsoleColor(BRIGHT_YELLOW);
    }
    cout << "确定要退出吗? (Y/N): ";
    if (!ansi) {
        cout << "\033[0m";
    } else {
        ResetConsoleColor();
    }
    
    int ch = _getch();
    cout << (ch == 'Y' || ch == 'y' ? "Y" : "N") << endl;
    return (ch == 'Y' || ch == 'y');
}

void show_error(const string& msg, bool ansi) {
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



// 选择器，node是名称，long__是扩展长度，able_ansi是ANSI支持（反）
string chooser(string node, int long__, bool disable_ansi) {
    int px = find_title(node, total_lines, data);
    if (px == -1) {
        show_error("未找到菜单：" + node, !disable_ansi);
        return "#ERR";
    }

    int cc = countchar(data[px][4], ',');
    if (cc <= 0) {
        show_error("菜单 " + node + " 无有效选项", !disable_ansi);
        return "#ERR";
    }

    chooser_row = px;
    string text[cc+3];
    int x = setnum(data[px][2]);
    int y = setnum(data[px][3]);

    // 检查坐标有效性
    if (x == -1 || y == -1) {
        show_error("菜单 " + node + " 坐标配置无效", !disable_ansi);
        return "#ERR";
    }

    COORD original_pos = getxy();
    
    // 分割选项
    for (int i = 0; i <= cc; i++) {
        text[i] = fag(data[px][4], ',', i);
        // 去除前后空格
        text[i].erase(0, text[i].find_first_not_of(" \t"));
        text[i].erase(text[i].find_last_not_of(" \t") + 1);
    }
    
    string huanchong = "";
    bool change = true;
    int nowline = 0;

    // 隐藏光标
    SetConsoleCursorVisible(false);
	// 1. 获取标准输入句柄（控制台输入）
    HANDLE hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdInput == INVALID_HANDLE_VALUE)
    {
        std::cerr << "错误：获取控制台输入句柄失败！" << std::endl;
        system("pause");
        return "#ERR";
    }

    // 2. 保存原始控制台输入模式（用于程序退出时恢复）
    DWORD dwOriginalInputMode = 0;
    if (!GetConsoleMode(hStdInput, &dwOriginalInputMode))
    {
        std::cerr << "错误：获取控制台输入模式失败！" << std::endl;
        system("pause");
        return "#ERR";
    }

    // 3. 设置新的控制台输入模式：启用鼠标输入捕获
    // ENABLE_MOUSE_INPUT：允许捕获鼠标事件
    // ENABLE_EXTENDED_FLAGS：配合鼠标输入启用，确保事件正常上报
    DWORD dwNewInputMode = dwOriginalInputMode | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS;
    if (!SetConsoleMode(hStdInput, dwNewInputMode))
    {
        std::cerr << "错误：设置控制台鼠标输入模式失败！" << std::endl;
        system("pause");
        return "#ERR";
    }
    // 4. 提示信息
    // 5. 循环处理控制台输入事件（核心：捕获鼠标滚轮）
    INPUT_RECORD inputRecord[128]; // 存储输入事件缓冲区
    DWORD dwEventsRead = 0;         // 实际读取的事件数
    bool is_choiced = 0;
    
    while (!is_choiced) {
        huanchong = "";
        is_choiced = 0;
        gotoxy(x, y);
        int tmp = 0;
        if (change) {
        	paint_list(x, y, nowline, cc, text, long__, disable_ansi, huanchong);
        }
        change = false;
        
        // 非阻塞读取控制台输入事件（避免程序卡死）
        if (PeekConsoleInput(hStdInput, inputRecord, 128, &dwEventsRead))
        {
            if (dwEventsRead > 0)
            {
                // 读取并处理所有待处理事件
                ReadConsoleInput(hStdInput, inputRecord, 128, &dwEventsRead);

                for (DWORD i = 0; i < dwEventsRead; i++)
                {
                    // 区分事件类型
                    switch (inputRecord[i].EventType)
                    {
                        // 鼠标事件（包含滚轮事件）
                        case MOUSE_EVENT:
                            tmp = ProcessConsoleMouseEvent(inputRecord[i].Event.MouseEvent);
                            if(tmp > 0 && nowline > 0){
                            	nowline--;
							}else if(tmp < 0 && nowline < cc - 1){
                            	nowline++;
							}
                           	change = true;
							break;
                        // 键盘事件
                        case KEY_EVENT:
                            if (inputRecord[i].Event.KeyEvent.bKeyDown)
                            {
                            	// 提取按键字符，支持大写A（0x41）和小写a（0x61）
                                WORD keyChar = inputRecord[i].Event.KeyEvent.wVirtualKeyCode;
								// 上下键事件同理
								if (keyChar == VK_UP && nowline > 0) {
								    nowline--;
								}
								else if (keyChar == VK_DOWN && nowline < cc - 1) {
								    nowline++;
								}
								else if (keyChar == VK_RETURN) {
									is_choiced = 1; 
								}
								else if (keyChar == VK_ESCAPE) {
									nowline = -1;
									is_choiced = 1; 
								}
								change = true;
                            }
                            break;

                        // 其他事件（忽略）
                        default:
                            break;
                    }
                	Sleep(1);
                }
            }
        }

        // 短暂延时，降低CPU占用
        Sleep(10);
        /*
        // ========== 关键修改4：非阻塞检测键盘输入 ==========
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 224) {
                ch = _getch();
                switch (ch) {
                    case 72: // 上箭头
                        if (nowline > 0) {
                            nowline--;
                            change = true;
                        }
                        break;
                    case 80: // 下箭头
                        if (nowline < cc - 1) {
                            nowline++;
                            change = true;
                        }
                        break;
                }
            } else if (ch == 13) { // 回车
                break;
            } else if (ch == 27) { // ESC
                nowline = -1;
                break;
            }
        } else {
            // 无输入时短暂休眠，降低CPU占用
            Sleep(10);
        }
    }

    // ========== 关键修改5：恢复输入模式 ==========
    setConsoleInputMode(false);

	//*/
	}
	
    // 原有清理逻辑保持不变
    int clear_lines = 0;
    if (cc <= 2 * long__ + 1 || long__ < 0) {
        clear_lines = cc + 4;
    } else {
        clear_lines = 2 * long__ + 5;
    }

    gotoxy(x, y);
    for (int i = 0; i < clear_lines; i++) {
        cout << string(80, ' ') << endl;
    }

    gotoxy(original_pos.X, original_pos.Y);
    SetConsoleCursorVisible(true);

    if (nowline == -1) {
        return "#ESC";
    }
    if (nowline >= 0 && nowline < cc) {
        if (text[nowline + 1] == "#ESC") {
            return "#ESC";
        }
        return text[nowline + 1];
    }
    return "#ERR";
}



bool cover_up() {
    int px = find_title("#自定义覆盖", total_lines, data);
    if (px == -1) {
        return false;
    }

    int sep_count = countchar(data[px][4], ';');
    for (int i = 0; i <= sep_count; i++) {
        cover[i] = fag(data[px][4], ';', i);
        // 去除前后空格
        cover[i].erase(0, cover[i].find_first_not_of(" \t"));
        cover[i].erase(cover[i].find_last_not_of(" \t") + 1);
        
        if (!cover[i].empty() && cover[i][0] == '#') {
            cover[i] = "";
            return true;
        }
    }
    return false;
}

string get_ftl(string node, string son) {
    int ftl_count = cne(ftl, 10001);
    for (int i = 0; i < ftl_count; i++) {
        string n = fag(ftl[i], '>', 0);
        string s = fag(ftl[i], '>', 1);
        if (n == node && s == son) {
            return fag(ftl[i], '>', 2);
        }
    }
    return "#ERR";
}

bool set_ftl(string node, string son, string next) {
    int px = -1;
    for (int i = 0; i < 10001; i++) {
        if (ftl[i].empty()) {
            px = i;
            break;
        }
    }
    if (px == -1) {
        show_error("ftl数组已满，无法添加新条目", false);
        return false;
    }
    ftl[px] = node + ">" + son + ">" + next;
    return true;
}

int main() {
    // 初始化FTL
    //set_ftl("初始选项", "左键连点", "数据存储1"); 
    set_ftl("数据存储1", "6666", "#ESC"); 
    set_ftl("初始选项", "退出程序", "#ESC"); 
    set_ftl("初始选项", "设置", "设置"); 
    set_ftl("设置", "美化选择器线条", "设置线条样式"); 
    set_ftl("初始选项", "左键连点", "左键连点"); 
    set_ftl("设置", "返回", "#ESC");  
    set_ftl("设置线条样式", "返回", "#ESC");  
    set_ftl("左键连点", "返回", "#ESC"); 
    for (int i = 0; i < 15; i++) {
        set_ftl("", "", ""); 
    }

    // 1. 读取 settings.dll 文件并获取行数
    total_lines = readSettings();
    bool enable_ansi = true;
    printf("连点器v");
    printf(setstr(A_VERSION).c_str());
    printf(".");
    printf(setstr(B_VERSION).c_str());
    printf(".");
    printf(setstr(C_VERSION).c_str());
	printf("重置版--Build Succeed at : 02/01/26 20:34\n检查ANSI支持...");
    gotoxy(0, 2); 
    Loader loader;
    gotoxy(0, 1); 
	show_long = setnum(data[find_title("这是数据存储-showlong",total_lines,data)][4]);
    // 检测并启用ANSI
    if (!supportsAnsiEscapes()) {
        enableAnsiOnWindows();
        if (!supportsAnsiEscapes()) {
            SetConsoleColor(BRIGHT_RED);
            std::cout << "\r无法启用 ANSI 转义码支持\n" << std::endl;
            ResetConsoleColor();
            Sleep(3000);
            enable_ansi = false;
        }
    }

    // DEBUG模式提示
    #ifdef DEBUG
        SetConsoleColor(BRIGHT_GREEN);
        gotoxy(60, 0);
        printf("| DEBUG模式运行\n");
        ResetConsoleColor();
    #endif
    string __tmp = "";
    if(data[find_title("New",total_lines,data)][4] == "True")
	{
		__tmp = chooser("Helps1",show_long,!enable_ansi);
		if(__tmp == "回车确认选项"){
			__tmp = chooser("Helps2",show_long,!enable_ansi);
			if(__tmp != "完成"){
				return 0;
			}else{
				change_data("New","False");
			} 
		}else if(__tmp == "跳过所有？"){
			change_data("New","False");
		}else{
			return 0;
		}
	} 
    
	if(A_VERSION == 5 && B_VERSION == 1){
		__tmp = chooser("tip",show_long,!enable_ansi);
		if(__tmp == "确认"){
			cout << "见谅！\r";
			Sleep(100);
		}else{
			return 0;
		}
	}
    

    // 隐藏光标
    SetConsoleCursorVisible(false);

    // 主循环变量
    string current_menu = "初始选项";
    string menu_path[10000];  // 存储导航路径的数组
    int path_index = 0;
    menu_path[path_index] = current_menu;  // 初始化路径

    // 主循环实现层级导航
    while (true) {
        gotoxy(0, 1);
        // 显示当前路径
        if (!enable_ansi) {
            cout << "\033[96m"; // 亮青色
        } else {
            SetConsoleColor(BRIGHT_CYAN);
        }
        cout << "路径: ";
        for (int i = 0; i <= path_index; i++) {
            cout << menu_path[i];
            if (i < path_index) cout << " > ";
        }
        cout << string(80 - (10 + path_index * 3), ' ') << endl << endl;
        if (!enable_ansi) {
            cout << "\033[0m";
        } else {
            ResetConsoleColor();
        }

        // 显示当前菜单
        string selected = chooser(current_menu, show_long, !enable_ansi);
        
        if (selected == "#ESC") {
            if (path_index > 0) {
                // 返回上一级
                path_index--;
                current_menu = menu_path[path_index];
                // 清除后续路径
                for (int i = path_index + 1; i < 100; i++) {
                    menu_path[i] = ""; 
                }
            } else {
                // 退出确认
                if (confirm_exit(enable_ansi)) break;
            }
        } else if (selected == "#ERR") {
            show_error("无效选项!", enable_ansi);
            Sleep(1000);
        } else {
            string next_menu = get_ftl(current_menu, selected);
            if (next_menu == "#ERR") {
                // 执行终端操作
                execute_command(setstr(chooser_row) + " " + selected, enable_ansi);
            } else if (next_menu == "#ESC") {
                if (path_index <= 0) {
                    // 退出确认
                    if (confirm_exit(enable_ansi)) break;
                } else {
                    // 返回上一级
                    path_index--;
                    current_menu = menu_path[path_index];
                    // 清除后续路径
                    for (int i = path_index + 1; i < 100; i++) {
                        menu_path[i] = ""; 
                    }
                }
            } else {
                // 进入子菜单
                if (path_index < 99) {
                    path_index++;
                    menu_path[path_index] = next_menu;
                    current_menu = next_menu;
                } else {
                    show_error("路径深度达到上限!", enable_ansi);
                }
            }
        }
    }

    // 恢复光标显示
    SetConsoleCursorVisible(true);
	
	uninstallHook();
    // 3. 程序结束时保存文件
    saveSettings(total_lines);
    return 0; // 改为正常返回值，原返回值3221225477对应0xC0000005（访问违规），不合理
}
