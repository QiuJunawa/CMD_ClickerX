#include "mytools.h"
#include <windows.h>
#include <wincrypt.h>
#include <iomanip>
#include <sstream>
#include <bcrypt.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "bcrypt.lib")

// ===================== ANSI 支持 =====================
bool supportsAnsiEscapes() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (!GetConsoleMode(hConsole, &mode)) {
        return false;
    }
    return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}

void enableAnsiOnWindows() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hConsole, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, mode);
    }
}

// ===================== 控制台操作 =====================
void gotoxy(int x, int y) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord;
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    SetConsoleCursorPosition(hConsole, coord);
}

COORD getxy() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        return csbi.dwCursorPosition;
    }
    COORD invalid = {0, 0};
    return invalid;
}

void SetConsoleColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void ResetConsoleColor() {
    SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void SetConsoleCursorVisible(bool visible) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hConsole, &cci);
    cci.bVisible = visible;
    SetConsoleCursorInfo(hConsole, &cci);
}

// ===================== 工具函数 =====================
int set0(int tmp) {
    if (tmp < 0) return 0;
    return tmp;
}

int set_max(int tmp, int max_val) {
    if (tmp > max_val) return max_val;
    return tmp;
}

int set_min(int tmp, int min_val) {
    if (tmp < min_val) return min_val;
    return tmp;
}

std::string find_data(std::string node, int px, int total, std::string data[][10]) {
    for (int i = 0; i < total; i++) {
        if (data[i][1] == node) {
            std::string list = data[i][4];
            std::vector<std::string> items;
            std::stringstream ss(list);
            std::string item;
            while (getline(ss, item, ',')) {
                items.push_back(item);
            }
            if (px >= 0 && px < (int)items.size()) {
                return items[px];
            }
            return "";
        }
    }
    return "";
}

int find_title(const std::string& title, int total, const std::string data[][10]) {
    for (int i = 0; i < total; i++) {
        if (data[i][1] == title) {
            return i;
        }
    }
    return -1;
}

void CleanLine(int start, int end) {
    if (start > end) std::swap(start, end);  // 确保范围有效

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        return;  // 无法获取信息，放弃
    }

    int width = csbi.dwSize.X;  // 控制台列数
    std::string clearLine(width, ' ');  // 一行空格

    for (int y = start; y <= end; ++y) {
        COORD pos = {0, (SHORT)y};
        SetConsoleCursorPosition(hConsole, pos);
        std::cout << clearLine << std::flush;
    }
}


std::string calculate_hmac(const std::string& password, const std::string& key) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    DWORD cbHash = 0;
    DWORD cbData = 0;
    std::string result;

    // 1. 打开 HMAC-SHA256 算法提供者
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (!BCRYPT_SUCCESS(status)) return "";

    // 2. 获取哈希长度
    BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(cbHash), &cbData, 0);

    // 3. 创建哈希对象，传入 HMAC 密钥
    status = BCryptCreateHash(hAlg, &hHash, NULL, 0, (PBYTE)key.c_str(), key.size(), 0);
    if (!BCRYPT_SUCCESS(status)) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    // 4. 添加密码数据
    status = BCryptHashData(hHash, (PBYTE)password.c_str(), password.size(), 0);
    if (!BCRYPT_SUCCESS(status)) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    // 5. 获取最终哈希
    std::vector<BYTE> hash(cbHash);
    status = BCryptFinishHash(hHash, hash.data(), cbHash, 0);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (!BCRYPT_SUCCESS(status)) return "";

    // 6. 转换为十六进制字符串
    std::ostringstream oss;
    for (DWORD i = 0; i < cbHash; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
}

std::string getMachineGuid() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "SOFTWARE\\Microsoft\\Cryptography",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[64] = {0};
        DWORD size = sizeof(buffer);
        if (RegQueryValueExA(hKey, "MachineGuid", nullptr, nullptr,
                             (LPBYTE)buffer, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buffer, size - 1); // 去掉末尾空字符
                             }
        RegCloseKey(hKey);
                      }
    // 如果读取失败，返回一个固定值（但仍可工作）
    return "DEFAULT_GUID";
}