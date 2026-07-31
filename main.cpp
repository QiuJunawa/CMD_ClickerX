#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
#include <conio.h>
#include <thread>
#include "mytools.h"
#include "runner.h"
#include "selector.h"

using namespace std;

// 全局数据
string data[10000][10];
string cover[10001], ftl[10001];
int total_lines = 0;
int g_menusize = 1;

// 全局变量
// 解析
std::string g_password_hmac;  // 存储 HMAC 的十六进制字符串
const std::string SECRET_KEY = getMachineGuid();  // 硬编码密钥，可混淆
// 密码输入（隐藏输入，显示星号）
std::string getPasswordInput(bool showAsterisk = true);
// main.cpp 全局数据下方添加
int g_hotkeyId = -1;
int g_hotkeyVkCode = VK_F1; // 默认热键F1

// 前向声明
string setstr(int num);
int findTitle(const string& title);
int countchar(const string& str, char target);
string fag(const string& str, char delimiter, size_t index);
string get_ftl(string node, string son);
bool set_ftl(string node, string son, string next);
bool confirm_exit(bool ansi);
void show_error(const string& msg, bool ansi);


// ===================== 工具函数实现 =====================

int findTitle(const string& title) {
    for (int i = 0; i < total_lines; i++) {
        if (data[i][1] == title) {
            return i;
        }
    }
    return -1;
}

string setstr(int num) {
    if (num == 0) return "0";
    string ret;
    bool negative = false;
    if (num < 0) {
        negative = true;
        num = -num;
    }
    while (num > 0) {
        ret.push_back('0' + (num % 10));
        num /= 10;
    }
    reverse(ret.begin(), ret.end());
    if (negative) ret.insert(ret.begin(), '-');
    return ret;
}

int setnum(string num) {
    int rnum = 0;
    for (char c : num) {
        if (c < '0' || c > '9') return -1;
        rnum = rnum * 10 + (c - '0');
    }
    return rnum;
}

string fag(const string& str, char delimiter, size_t index) {
    vector<string> tokens;
    string token;
    size_t pos = 0, last_pos = 0;
    while (pos <= str.length()) {
        pos = str.find(delimiter, last_pos);
        token = str.substr(last_pos, pos - last_pos);
        tokens.push_back(token);
        last_pos = pos + 1;
    }
    if (index < tokens.size()) return tokens[index];
    return "";
}

int countchar(const string& str, char target) {
    return count(str.begin(), str.end(), target);
}

string get_ftl(string node, string son) {
    for (int i = 0; i < 10001; i++) {
        if (ftl[i].empty()) continue;
        string n = fag(ftl[i], '>', 0);
        string s = fag(ftl[i], '>', 1);
        if (n == node && s == son) {
            return fag(ftl[i], '>', 2);
        }
    }
    return "#ERR";
}

bool set_ftl(string node, string son, string next) {
    for (int i = 0; i < 10001; i++) {
        if (ftl[i].empty()) {
            ftl[i] = node + ">" + son + ">" + next;
            return true;
        }
    }
    return false;
}

bool confirm_exit(bool ansi) {
    if (!ansi) {
        cout << "\033[93m";
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
        cerr << "\033[91m";
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

// ===================== 菜单系统 =====================
class MenuSystem {
    string current_menu_;
    string menu_path_[100];
    int path_index_;
    bool ansi_enabled_;
    int window_size_;

public:
    MenuSystem(bool ansi = false, int windowSize = 3)
        : current_menu_("初始选项"), path_index_(0),
          ansi_enabled_(ansi), window_size_(windowSize) {
        menu_path_[0] = current_menu_;
        initFTL();
    }

    // 初始化 FTL（跳转表）
    void initFTL() {
        set_ftl("初始选项", "退出程序", "#ESC");
        set_ftl("初始选项", "设置", "设置");
        set_ftl("初始选项", "自定义按键", "自定义");
        set_ftl("设置", "美化选择器线条", "设置线条样式");
        set_ftl("初始选项", "左键连点", "左键连点");
        set_ftl("初始选项", "中键连点", "中键连点");
        set_ftl("初始选项", "右键连点", "右键连点");
        set_ftl("设置", "返回", "#ESC");
        set_ftl("设置线条样式", "返回", "#ESC");
        set_ftl("左键连点", "返回", "#ESC");
        set_ftl("中键连点", "返回", "#ESC");
        set_ftl("右键连点", "返回", "#ESC");
        set_ftl("自定义", "返回", "#ESC");
    }

    // 运行主循环
    void run() {
        while (true) {
            // 显示路径
            displayPath();

            // 显示当前菜单
            string selected = showMenu();

            // 处理选择
            if (!handleSelection(selected)) {
                break;  // 退出
            }
        }
    }

private:
    void displayPath() {
        gotoxy(0, 1);
        if (ansi_enabled_) {
            cout << "\033[96m";
        } else {
            SetConsoleColor(BRIGHT_CYAN);
        }

        cout << "路径: ";
        for (int i = 0; i <= path_index_; i++) {
            cout << menu_path_[i];
            if (i < path_index_) cout << " > ";
        }
        cout << string(80 - (10 + path_index_ * 3), ' ') << endl << endl;

        if (ansi_enabled_) {
            cout << "\033[0m";
        } else {
            ResetConsoleColor();
        }
    }

    string showMenu() {
        int px = findTitle(current_menu_);
        if (px == -1) {
            show_error("未找到菜单: " + current_menu_, ansi_enabled_);
            system("pause");
            return "#ERR";
        }

        // 构建选项列表
        vector<string> items;
        items.push_back(data[px][1]);  // 标题

        // 解析选项
        string options_str = data[px][4];
        stringstream ss(options_str);
        string option;
        while (getline(ss, option, ',')) {
            // 去除前后空格
            option.erase(0, option.find_first_not_of(" \t"));
            option.erase(option.find_last_not_of(" \t") + 1);
            if (!option.empty()) {
                items.push_back(option);
            }
        }

        if (items.size() < 2) {
            show_error("菜单 " + current_menu_ + " 无有效选项", ansi_enabled_);
            return "#ERR";
        }

        // 获取坐标
        int x = setnum(data[px][2]);
        int y = setnum(data[px][3]);
        if (x == -1 || y == -1) {
            show_error("菜单 " + current_menu_ + " 坐标配置无效", ansi_enabled_);
            return "#ERR";
        }

        // 使用高性能选择器
        Selector selector(items, x, y, g_menusize * 2 + 1 <= items.size() - 2 ? g_menusize * 2 + 1 : items.size() - 2, ansi_enabled_);
        int selected = selector.run();

        if (selected == -1) {
            return "#ESC";
        }

        return selector.getSelectedText();
    }

    bool handleSelection(const string& selected) {
        if (selected == "#ESC") {
            return handleEscape();
        } else if (selected == "#ERR") {
            return true;  // 继续运行
        } else {
            return handleOption(selected);
        }
    }

    bool handleEscape() {
        if (path_index_ > 0) {
            // 返回上一级
            path_index_--;
            current_menu_ = menu_path_[path_index_];
            for (int i = path_index_ + 1; i < 100; i++) {
                menu_path_[i] = "";
            }
            return true;
        } else {
            // 退出确认
            return !confirm_exit(ansi_enabled_);
        }
    }

    bool handleOption(const string& selected) {
        string next_menu = get_ftl(current_menu_, selected);

        if (next_menu == "#ERR") {
            // 执行命令
            int row = findTitle(current_menu_);
            execute_command(current_menu_, selected, ansi_enabled_);
            return true;
        } else if (next_menu == "#ESC") {
            return handleEscape();
        } else {
            // 进入子菜单
            if (path_index_ < 99) {
                path_index_++;
                menu_path_[path_index_] = next_menu;
                current_menu_ = next_menu;
                return true;
            } else {
                show_error("路径深度达到上限!", ansi_enabled_);
                return true;
            }
        }
    }
};

// ===================== 文件操作 =====================

int readSettings() {
    string file_path = "settings.txt";
    ifstream file(file_path);
    if (!file.is_open()) {
        cerr << "无法打开 " << file_path << " 文件，将创建新文件" << endl;
        system("pause");
        return 0;
    }

    string line;
    int lines_read = 0;
    while (getline(file, line) && lines_read < 10000) {
        for (int j = 1; j < 10; j++) data[lines_read][j] = "";

        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty()) continue;

        // 修改 readSettings 解析
        if (line.find("PASSWORD_HMAC ") == 0) {
            g_password_hmac = line.substr(14);  // 长度 "PASSWORD_HMAC " 为14
            continue;
        }

        if (line.find("EMPTY_PASSWORD") == 0) {
            continue;
        }

        if (line.find("WINDOWSIZE ") == 0) {
            std::string numStr = line.substr(10);
            numStr.erase(0, numStr.find_first_not_of(" \t"));
            numStr.erase(numStr.find_last_not_of(" \t") + 1);
            if (!numStr.empty() && std::all_of(numStr.begin(), numStr.end(), ::isdigit)) {
                g_menusize = setnum(numStr);
            }
            continue;
        }

        if (line.find("HOTKEY ") == 0) {
            g_hotkeyVkCode = setnum(line.substr(7));
            if (g_hotkeyVkCode == -1) {
                g_hotkeyVkCode = VK_F1;
            }
            continue;
        }

        size_t pos1 = line.find(' ');
        if (pos1 == string::npos) {
            data[lines_read][1] = line;
            lines_read++;
            continue;
        }
        data[lines_read][1] = line.substr(0, pos1);

        size_t pos2 = line.find(' ', pos1 + 1);
        if (pos2 == string::npos) {
            data[lines_read][2] = line.substr(pos1 + 1);
            lines_read++;
            continue;
        }
        data[lines_read][2] = line.substr(pos1 + 1, pos2 - pos1 - 1);

        size_t pos3 = line.find(' ', pos2 + 1);
        if (pos3 == string::npos) {
            data[lines_read][3] = line.substr(pos2 + 1);
            lines_read++;
            continue;
        }
        data[lines_read][3] = line.substr(pos2 + 1, pos3 - pos2 - 1);
        data[lines_read][4] = line.substr(pos3 + 1);
        lines_read++;
    }
    file.close();
    return lines_read;
}

void saveSettings(int total_lines) {
    string file_path = "settings.txt";
    ofstream file(file_path);
    if (!file.is_open()) {
        cerr << "无法创建 " << file_path << " 文件" << endl;
        return;
    }
    for (int i = 0; i < total_lines; i++) {
        bool first = true;
        for (int j = 1; j < 10; j++) {
            if (!data[i][j].empty()) {
                if (!first) file << " ";
                file << data[i][j];
                first = false;
            }
        }
        file << endl;
    }
    if (!g_password_hmac.empty()) {
        file << "PASSWORD_HMAC " << g_password_hmac << endl;
    }else {
        file << "EMPTY_PASSWORD" << endl;
    }
    file << "WINDOWSIZE " << g_menusize << endl;
    file << "HOTKEY " << g_hotkeyVkCode << endl;
    file.close();
}

std::string getPasswordInput(bool showAsterisk) {
    std::string pwd;
    char ch;
    while (true) {
        ch = _getch();
        if (ch == 13) break;          // 回车结束输入
        if (ch == 8) {                // 退格
            if (!pwd.empty()) {
                pwd.pop_back();
                if (showAsterisk) {
                    std::cout << "\b \b";   // 删除一个星号
                }
            }
        } else if (ch >= 32 && ch <= 126) { // 可打印字符
            pwd.push_back(ch);
            if (showAsterisk) {
                std::cout << '*';
            }
        }
    }
    std::cout << std::endl;
    return pwd;
}

bool verifyPassword() {
    if (g_password_hmac.empty()) {
        return true;   // 未设置密码
    }
    std::cout << "请输入密码: ";
    std::string input = getPasswordInput(true);
    std::string hmac = calculate_hmac(input, SECRET_KEY);
    return hmac == g_password_hmac;
}

void setNewPassword() {
    std::cout << "请输入新密码 (回车确认): ";
    std::string pwd1 = getPasswordInput(true);
    if (pwd1.empty()) {
        g_password_hmac.clear();
        std::cout << "密码已清除。" << std::endl;
    } else {
        std::cout << "请再次输入确认: ";
        std::string pwd2 = getPasswordInput(true);
        if (pwd1 == pwd2) {
            g_password_hmac = calculate_hmac(pwd1, SECRET_KEY);
            std::cout << "密码已设置。" << std::endl;
        } else {
            std::cout << "两次输入不一致，密码未更改。" << std::endl;
        }
    }
    saveSettings(total_lines);
}

// ===================== 主程序 =====================

int main() {
    //#define DEBUG
    // 设置控制台编码
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    #endif

    // 读取配置
    total_lines = readSettings();

    // 检测 ANSI 支持
    bool enable_ansi = true;
    if (!supportsAnsiEscapes()) {
        enableAnsiOnWindows();
        if (!supportsAnsiEscapes()) {
            SetConsoleColor(BRIGHT_RED);
            cout << "\r无法启用 ANSI 转义码支持" << endl;
            ResetConsoleColor();
            Sleep(3000);
            enable_ansi = false;
        }
    }

    cout << "连点器v6.0.0重置版 -- 成功构建2026年7月31日14:10:27" << endl;

    #ifdef DEBUG
    SetConsoleColor(BRIGHT_GREEN);
    gotoxy(40, 0);
    printf("| debug-mode模式运行\n");
    ResetConsoleColor();
    // 检查当前目录是否有 settings.txt
    ifstream test1("settings.txt");
    if (test1.is_open()) {
        cout << "[OK] 当前目录找到 settings.txt" << endl;
        test1.close();
    } else {
        cout << "[FAIL] 当前目录未找到 settings.txt" << endl;
    }
    cout << total_lines << endl;
    for (int i = 0;i <= total_lines;i++) {
        cout << data[i][1] << endl;
    }
    cout << endl << getMachineGuid() << endl;
    system("pause");
    #endif

    if (!verifyPassword()) {
        cout << "密码错误";
        return 0;
    }

    // 创建并运行菜单系统
    MenuSystem menu(enable_ansi, 6);
    menu.run();

    // 保存配置
    saveSettings(total_lines);
    return 0;
}