#include "runner.h"
#include "Selector.h"
#include <thread>
#include <chrono>

using namespace std;

// 外部函数声明（这些在 main.cpp 中实现）
extern bool verifyPassword();
extern void setNewPassword();
extern void saveSettings(int);
extern void readSettings();
extern int total_lines;
extern int g_menusize;

string keyName = "f1";

// ===================== 通用命令函数（可复用） =====================

// 通用执行函数：打印信息
void printAction(const string& menuName, const string& option, bool ansi) {
    if (ansi) SetConsoleColor(BRIGHT_GREEN);
    cout << "▶ 执行: [" << menuName << "] " << option << endl;
    if (ansi) ResetConsoleColor();
}

// 通用退出函数
void doExit(const string& menuName, const string& option, bool ansi) {
    if (ansi) SetConsoleColor(BRIGHT_YELLOW);
    cout << "▶ 返回/退出: [" << menuName << "] " << option << endl;
    if (ansi) ResetConsoleColor();
}

// 通用设置函数
void doSettings(const string& menuName, const string& option, bool ansi) {
    if (ansi) SetConsoleColor(BRIGHT_BLUE);
    cout << "▶ 设置: [" << menuName << "] " << option << endl;
    if (ansi) ResetConsoleColor();
}
// ---------- 辅助函数：虚拟键码转名称 ----------
auto vkToName = [](int vk) -> string {
    // 可参考之前的 vkToName，或使用 MapVirtualKey 获取键名
    char name[64] = {0};
    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    if (scanCode) {
        if (GetKeyNameTextA(scanCode << 16, name, sizeof(name)) > 0) {
            return string(name);
        }
    }
    // 如果失败，使用硬编码名称
    switch (vk) {
        case VK_F1: return "F1"; case VK_F2: return "F2";
        case VK_F3: return "F3"; case VK_F4: return "F4";
        case VK_F5: return "F5"; case VK_F6: return "F6";
        case VK_F7: return "F7"; case VK_F8: return "F8";
        case VK_F9: return "F9"; case VK_F10: return "F10";
        case VK_F11: return "F11"; case VK_F12: return "F12";
        case VK_UP: return "↑"; case VK_DOWN: return "↓";
        case VK_LEFT: return "←"; case VK_RIGHT: return "→";
        case VK_HOME: return "Home"; case VK_END: return "End";
        case VK_PRIOR: return "PageUp"; case VK_NEXT: return "PageDown";
        case VK_INSERT: return "Insert"; case VK_DELETE: return "Delete";
        case VK_RETURN: return "Enter";
        case VK_TAB: return "Tab";
        case VK_SPACE: return "Space";
        case VK_OEM_3: return "`";
        case VK_OEM_4: return "[";
        case VK_OEM_6: return "]";
        case VK_OEM_5: return "\\";
        case VK_OEM_1: return ";";
        case VK_OEM_7: return "'";
        case VK_OEM_COMMA: return ",";
        case VK_OEM_PERIOD: return ".";
        case VK_OEM_2: return "/";
        default:
            if (vk >= 'A' && vk <= 'Z') return string(1, (char)vk);
            if (vk >= '0' && vk <= '9') return string(1, (char)vk);
            return "未知键(" + to_string(vk) + ")";
    }
};

HotkeyManager g_hotkeyManager;

// ===================== 命令表（使用 Lambda 绑定具体行为） =====================

CommandEntry command_table[] = {
    // ===== 全局命令（菜单名为空，匹配所有菜单） =====
    {"", "退出程序", 
        [](const string& menu, const string& opt, bool ansi) {
            doExit(menu, opt, ansi);
            // 这里可以添加退出前的清理逻辑
        }
    },
    {"", "返回", 
        [](const string& menu, const string& opt, bool ansi) {
            doExit(menu, opt, ansi);
        }
    },
    
    // ===== 初始选项菜单 =====
    {"初始选项", "左键连点", 
        [](const string& menu, const string& opt, bool ansi) {
            printAction(menu, opt, ansi);
            cout << "  → 启动左键连点 (点击间隔: 100ms)" << endl;
            // 这里添加左键连点逻辑
        }
    },
    {"初始选项", "中键连点", 
        [](const string& menu, const string& opt, bool ansi) {
            printAction(menu, opt, ansi);
            cout << "  → 启动中键连点" << endl;
        }
    },
    {"初始选项", "右键连点", 
        [](const string& menu, const string& opt, bool ansi) {
            printAction(menu, opt, ansi);
            cout << "  → 启动右键连点" << endl;
        }
    },
    {"初始选项", "设置", 
        [](const string& menu, const string& opt, bool ansi) {
            doSettings(menu, opt, ansi);
        }
    },
    
    // ===== 设置菜单 =====
    {"设置", "美化选择器线条", 
        [](const string& menu, const string& opt, bool ansi) {
            printAction(menu, opt, ansi);
            cout << "  → 切换选择器线条样式" << endl;
            // 切换线条样式
            static int style = 0;
            style = (style + 1) % 4;
            cout << "  → 当前样式: " << style << endl;
        }
    },
    {"设置", "显示选择路径", 
        [](const string& menu, const string& opt, bool ansi) {
            printAction(menu, opt, ansi);
            static bool showPath = true;
            showPath = !showPath;
            cout << "  → 路径显示: " << (showPath ? "开启" : "关闭") << endl;
        }
    },
    {"设置", "设置密码", 
        [](const string& menu, const string& opt, bool ansi) {
            printAction(menu, opt, ansi);
            setNewPassword();
            Sleep(1000);
            // 这里可以添加密码输入逻辑
        }
    },
    {"设置", "修改热键",
        [](const string& menu, const string& opt, bool ansi) {
            // ---------- 辅助函数：检测用户按下的第一个键（虚拟键码） ----------
            auto detectKeyPress = []() -> int {
                // 清空控制台输入缓冲区
                while (_kbhit()) _getch();

                // 等待所有按键释放
                bool anyPressed = true;
                while (anyPressed) {
                    anyPressed = false;
                    for (int vk = 0x01; vk <= 0xFE; ++vk) {
                        if (GetAsyncKeyState(vk) & 0x8000) {
                            anyPressed = true;
                            break;
                        }
                    }
                    if (anyPressed) Sleep(10);
                }

                // 等待一个新按键按下
                int pressedVk = 0;
                while (pressedVk == 0) {
                    for (int vk = 0x01; vk <= 0xFE; ++vk) {
                        if (GetAsyncKeyState(vk) & 0x8000) {
                            // 排除修饰键（可调整）
                            if (vk != VK_SHIFT && vk != VK_CONTROL && vk != VK_MENU && vk != VK_CAPITAL) {
                                pressedVk = vk;
                                break;
                            }
                        }
                    }
                    Sleep(10);
                }

                // 等待按键释放
                while (GetAsyncKeyState(pressedVk) & 0x8000) Sleep(10);
                return pressedVk;
            };

            // ---------- 1. 清理旧热键 ----------
            if (g_hotkeyId != -1) {
                g_hotkeyManager.unregisterHotkey(g_hotkeyId);
                g_hotkeyId = -1;
            }

            if (ansi) SetConsoleColor(BRIGHT_CYAN);
            cout << "\n==================== 热键配置 ====================\n";
            cout << "支持：任意按键（字母、数字、功能键、符号等）\n";
            cout << "按下 ESC 取消修改，其他按键设为新热键\n";
            cout << "请按下你想要绑定的热键：";
            if (ansi) ResetConsoleColor();

            // 检测按键（使用 GetAsyncKeyState）
            int vkCode = detectKeyPress();
            if (vkCode == VK_ESCAPE) {
                if (ansi) SetConsoleColor(BRIGHT_YELLOW);
                cout << "\n已取消热键修改，保留原有配置\n";
                if (ansi) ResetConsoleColor();
                Sleep(1200);
                return;
            }

            // 检查是否有效（不允许绑定某些系统键，如 Windows 键）
            if (vkCode == 0 || vkCode == VK_LWIN || vkCode == VK_RWIN) {
                if (ansi) SetConsoleColor(BRIGHT_RED);
                cout << "\n错误：该按键不支持绑定，请更换按键！\n";
                if (ansi) ResetConsoleColor();
                Sleep(1500);
                return;
            }

            // 获取键名
            keyName = vkToName(vkCode);

            // ---------- 4. 保存并注册新热键 ----------
            g_hotkeyVkCode = vkCode;

            saveSettings(total_lines);

            // ---------- 5. 成功提示 ----------
            SetConsoleColor(BRIGHT_GREEN);
            cout << "\n热键绑定成功！\n";
            cout << "虚拟键码 VK: " << g_hotkeyVkCode << "\n";
            cout << "当前绑定热键：" << keyName << "\n";
            ResetConsoleColor();
            system("pause");
            CleanLine(2, 11);
        }
    },
    {"设置", "选项窗口大小",
        [](const string& menu, const string& opt, bool ansi) {
            cout << "输入窗口大小(默认: 1): ";
            short size = 1;
            cin >> size;
            g_menusize = size>=0 ? size : 1;
            cout << "设置成功!";
        }
    },
    {"设置", "从文件刷新选项",
        [](const string& menu, const string& opt, bool ansi) {
            readSettings();
        }
    },
    
    // ===== 设置线条样式菜单 =====
    {"设置线条样式", "---/...", 
        [](const string& menu, const string& opt, bool ansi) {
            printAction(menu, opt, ansi);
            cout << "  → 应用样式: ---/..." << endl;
        }
    },
    {"设置线条样式", "===/---", 
        [](const string& menu, const string& opt, bool ansi) {
            printAction(menu, opt, ansi);
            cout << "  → 应用样式: ===/---" << endl;
        }
    },
    {"设置线条样式", "+++/---", 
        [](const string& menu, const string& opt, bool ansi) {
            printAction(menu, opt, ansi);
            cout << "  → 应用样式: +++/---" << endl;
        }
    },
    {"设置线条样式", "格式: 到底了/没到底",
        [](const string& menu, const string& opt, bool ansi) {
        }
    },
    
    // ===== 左键连点菜单 =====
    {"左键连点", "单次点击", 
        [](const string& menu, const string& opt, bool ansi) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            cout << "执行成功！" << endl;
            system("pause");
            CleanLine(2, 9);
        }
    },
    {"左键连点", "固定间隔点击",
    [](const string& menu, const string& opt, bool ansi) {
            cout << "输入间隔(ms): ";
            int interval = 500;
            cin >> interval;
            cout << "输入次数(ms): ";
            int c = 10;
            cin >> c;
            cout << "固定间隔点击 (间隔: " << interval << "ms)" << endl;
            keyName = vkToName(g_hotkeyVkCode);
            cout << "按 \"" << keyName << "\" 键启动连点，任意键退出当前页面（仅后台生效）\n";

            // 重置所有旧热键，避免冲突
            g_hotkeyManager.stop();
            g_hotkeyManager.resetAllHotkeys();
            // 注册F1热键
            g_hotkeyManager.registerHotkey(g_hotkeyVkCode, [interval, c]() {
                cout << "\r左键连点开始";
                INPUT down{}, up{};
                down.type = INPUT_MOUSE;
                down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                up.type = INPUT_MOUSE;
                up.mi.dwFlags = MOUSEEVENTF_LEFTUP;

                for (int i = 0; i < c; i++) {
                    SendInput(1, &down, sizeof(INPUT));
                    SendInput(1, &up, sizeof(INPUT));
                    Sleep(interval);
                }
                cout << "\r左键连点执行完毕";
            });
            g_hotkeyManager.start();

            // 非阻塞等待按键，不阻塞任何线程，后台热键持续工作
            while (true) {
                if (_kbhit()) {
                    _getch();
                    // 退出菜单停止热键
                    g_hotkeyManager.stop();
                    break;
                }
                Sleep(30);
            }
            cout << "已关闭全局热键监听\n";
            system("pause");
            CleanLine(2, 11);
        }
    },
    {"左键连点", "随机间隔点击", 
        [](const string& menu, const string& opt, bool ansi) {
            cout << "输入最小间隔(ms): ";
            int min_interval = 500;
            cin >> min_interval;
            cout << "输入最大间隔(ms): ";
            int max_interval = 500;
            cin >> max_interval;
            if (min_interval > max_interval) {
                swap(min_interval, max_interval);
            }
            cout << "输入次数(ms): ";
            int c = 10;
            cin >> c;
            cout << "  → 随机间隔点击 (间隔: " << min_interval << " ~ " << max_interval << "ms)" << endl;
            keyName = vkToName(g_hotkeyVkCode);
            cout << "按 \"" << keyName << "\" 键生成随机数并启动连点，任意键退出当前页面（仅后台生效）\n";

            // 重置所有旧热键，避免冲突
            g_hotkeyManager.stop();
            g_hotkeyManager.resetAllHotkeys();
            // 注册F1热键
            g_hotkeyManager.registerHotkey(g_hotkeyVkCode, [min_interval, max_interval, c]() {
                cout << "\r请稍等, 正在生成随机数...";
                // 1. 随机数生成器（使用随机设备播种）
                std::random_device rd;
                std::mt19937 gen(rd());
                // 2. 定义分布（这里生成 1~100 的整数）
                std::uniform_int_distribution<> dist(min_interval, max_interval);
                std::vector<int> now_interval(c);
                for (int i = 0; i < c; i++) {
                    // 3. 生成一个随机数
                    now_interval[i] = dist(gen);
                }
                cout << "\r[F1全局触发] 左键连点开始";
                INPUT down{}, up{};
                down.type = INPUT_MOUSE;
                down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                up.type = INPUT_MOUSE;
                up.mi.dwFlags = MOUSEEVENTF_LEFTUP;

                for (int i = 0; i < c; i++) {
                    SendInput(1, &down, sizeof(INPUT));
                    SendInput(1, &up, sizeof(INPUT));
                    Sleep(now_interval[i]);
                }
                cout << "\r[F1全局触发] 左键连点执行完毕";
            });
            g_hotkeyManager.start();

            // 非阻塞等待按键，不阻塞任何线程，后台热键持续工作
            while (true) {
                if (_kbhit()) {
                    _getch();
                    // 退出菜单停止热键
                    g_hotkeyManager.stop();
                    break;
                }
                Sleep(30);
            }
            cout << "已关闭全局热键监听\n";
            system("pause");
            CleanLine(2, 11);
        }
    },
    // ===== 中键连点菜单 =====
    {"中键连点", "单次点击", 
        [](const string& menu, const string& opt, bool ansi) {
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
            cout << "执行成功！" << endl;
            system("pause");
            CleanLine(2, 9);
        }
    },
    {"中键连点", "固定间隔点击",
    [](const string& menu, const string& opt, bool ansi) {
            cout << "输入间隔(ms): ";
            int interval = 500;
            cin >> interval;
            cout << "输入次数(ms): ";
            int c = 10;
            cin >> c;
            cout << "固定间隔点击 (间隔: " << interval << "ms)" << endl;
            keyName = vkToName(g_hotkeyVkCode);
            cout << "按 \"" << keyName << "\" 键启动连点，任意键退出当前页面（仅后台生效）\n";

            // 重置所有旧热键，避免冲突
            g_hotkeyManager.stop();
            g_hotkeyManager.resetAllHotkeys();
            // 注册F1热键
            g_hotkeyManager.registerHotkey(g_hotkeyVkCode, [interval, c]() {
                cout << "\r中键连点开始";
                INPUT down{}, up{};
                down.type = INPUT_MOUSE;
                down.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
                up.type = INPUT_MOUSE;
                up.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;

                for (int i = 0; i < c; i++) {
                    SendInput(1, &down, sizeof(INPUT));
                    SendInput(1, &up, sizeof(INPUT));
                    Sleep(interval);
                }
                cout << "\r中键连点执行完毕";
            });
            g_hotkeyManager.start();

            // 非阻塞等待按键，不阻塞任何线程，后台热键持续工作
            while (true) {
                if (_kbhit()) {
                    _getch();
                    // 退出菜单停止热键
                    g_hotkeyManager.stop();
                    break;
                }
                Sleep(30);
            }
            cout << "已关闭全局热键监听\n";
            system("pause");
            CleanLine(2, 11);
        }
    },
    {"中键连点", "随机间隔点击", 
        [](const string& menu, const string& opt, bool ansi) {
            cout << "输入最小间隔(ms): ";
            int min_interval = 500;
            cin >> min_interval;
            cout << "输入最大间隔(ms): ";
            int max_interval = 500;
            cin >> max_interval;
            if (min_interval > max_interval) {
                swap(min_interval, max_interval);
            }
            cout << "输入次数(ms): ";
            int c = 10;
            cin >> c;
            cout << "  → 随机间隔点击 (间隔: " << min_interval << " ~ " << max_interval << "ms)" << endl;
            keyName = vkToName(g_hotkeyVkCode);
            cout << "按 \"" << keyName << "\" 键生成随机数并启动连点，任意键退出当前页面（仅后台生效）\n";

            // 重置所有旧热键，避免冲突
            g_hotkeyManager.stop();
            g_hotkeyManager.resetAllHotkeys();
            // 注册F1热键
            g_hotkeyManager.registerHotkey(g_hotkeyVkCode, [min_interval, max_interval, c]() {
                cout << "\r请稍等, 正在生成随机数...";
                // 1. 随机数生成器（使用随机设备播种）
                std::random_device rd;
                std::mt19937 gen(rd());
                // 2. 定义分布（这里生成 1~100 的整数）
                std::uniform_int_distribution<> dist(min_interval, max_interval);
                std::vector<int> now_interval(c);
                for (int i = 0; i < c; i++) {
                    // 3. 生成一个随机数
                    now_interval[i] = dist(gen);
                }
                cout << "\r[F1全局触发] 中键连点开始";
                INPUT down{}, up{};
                down.type = INPUT_MOUSE;
                down.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
                up.type = INPUT_MOUSE;
                up.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;

                for (int i = 0; i < c; i++) {
                    SendInput(1, &down, sizeof(INPUT));
                    SendInput(1, &up, sizeof(INPUT));
                    Sleep(now_interval[i]);
                }
                cout << "\r[F1全局触发] 中键连点执行完毕";
            });
            g_hotkeyManager.start();

            // 非阻塞等待按键，不阻塞任何线程，后台热键持续工作
            while (true) {
                if (_kbhit()) {
                    _getch();
                    // 退出菜单停止热键
                    g_hotkeyManager.stop();
                    break;
                }
                Sleep(30);
            }
            cout << "已关闭全局热键监听\n";
            system("pause");
            CleanLine(2, 11);
        }
    },
    // ===== 右键连点菜单 =====
    {"右键连点", "单次点击", 
        [](const string& menu, const string& opt, bool ansi) {
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
            cout << "执行成功！" << endl;
            system("pause");
            CleanLine(2, 9);
        }
    },
    {"右键连点", "固定间隔点击",
    [](const string& menu, const string& opt, bool ansi) {
            cout << "输入间隔(ms): ";
            int interval = 500;
            cin >> interval;
            cout << "输入次数(ms): ";
            int c = 10;
            cin >> c;
            cout << "固定间隔点击 (间隔: " << interval << "ms)" << endl;
            keyName = vkToName(g_hotkeyVkCode);
            cout << "按 \"" << keyName << "\" 键启动连点，任意键退出当前页面（仅后台生效）\n";

            // 重置所有旧热键，避免冲突
            g_hotkeyManager.stop();
            g_hotkeyManager.resetAllHotkeys();
            // 注册F1热键
            g_hotkeyManager.registerHotkey(g_hotkeyVkCode, [interval, c]() {
                cout << "\r右键连点开始";
                INPUT down{}, up{};
                down.type = INPUT_MOUSE;
                down.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                up.type = INPUT_MOUSE;
                up.mi.dwFlags = MOUSEEVENTF_RIGHTUP;

                for (int i = 0; i < c; i++) {
                    SendInput(1, &down, sizeof(INPUT));
                    SendInput(1, &up, sizeof(INPUT));
                    Sleep(interval);
                }
                cout << "\r右键连点执行完毕";
            });
            g_hotkeyManager.start();

            // 非阻塞等待按键，不阻塞任何线程，后台热键持续工作
            while (true) {
                if (_kbhit()) {
                    _getch();
                    // 退出菜单停止热键
                    g_hotkeyManager.stop();
                    break;
                }
                Sleep(30);
            }
            cout << "已关闭全局热键监听\n";
            system("pause");
            CleanLine(2, 11);
        }
    },
    {"右键连点", "随机间隔点击", 
        [](const string& menu, const string& opt, bool ansi) {
            cout << "输入最小间隔(ms): ";
            int min_interval = 500;
            cin >> min_interval;
            cout << "输入最大间隔(ms): ";
            int max_interval = 500;
            cin >> max_interval;
            if (min_interval > max_interval) {
                swap(min_interval, max_interval);
            }
            cout << "输入次数(ms): ";
            int c = 10;
            cin >> c;
            cout << "  → 随机间隔点击 (间隔: " << min_interval << " ~ " << max_interval << "ms)" << endl;
            keyName = vkToName(g_hotkeyVkCode);
            cout << "按 \"" << keyName << "\" 键生成随机数并启动连点，任意键退出当前页面（仅后台生效）\n";

            // 重置所有旧热键，避免冲突
            g_hotkeyManager.stop();
            g_hotkeyManager.resetAllHotkeys();
            // 注册F1热键
            g_hotkeyManager.registerHotkey(g_hotkeyVkCode, [min_interval, max_interval, c]() {
                cout << "\r请稍等, 正在生成随机数...";
                // 1. 随机数生成器（使用随机设备播种）
                std::random_device rd;
                std::mt19937 gen(rd());
                // 2. 定义分布（这里生成 1~100 的整数）
                std::uniform_int_distribution<> dist(min_interval, max_interval);
                std::vector<int> now_interval(c);
                for (int i = 0; i < c; i++) {
                    // 3. 生成一个随机数
                    now_interval[i] = dist(gen);
                }
                cout << "\r[F1全局触发] 右键连点开始";
                INPUT down{}, up{};
                down.type = INPUT_MOUSE;
                down.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                up.type = INPUT_MOUSE;
                up.mi.dwFlags = MOUSEEVENTF_RIGHTUP;

                for (int i = 0; i < c; i++) {
                    SendInput(1, &down, sizeof(INPUT));
                    SendInput(1, &up, sizeof(INPUT));
                    Sleep(now_interval[i]);
                }
                cout << "\r[F1全局触发] 右键连点执行完毕";
            });
            g_hotkeyManager.start();

            // 非阻塞等待按键，不阻塞任何线程，后台热键持续工作
            while (true) {
                if (_kbhit()) {
                    _getch();
                    // 退出菜单停止热键
                    g_hotkeyManager.stop();
                    break;
                }
                Sleep(30);
            }
            cout << "已关闭全局热键监听\n";
            system("pause");
            CleanLine(2, 11);
        }
    },
};

int command_table_size = sizeof(command_table) / sizeof(CommandEntry);

// ===================== 执行命令 =====================
void execute_command(const string& menuName, const string& option, bool ansi) {
    // 1. 优先精确匹配（菜单名 + 选项名）
    for (int i = 0; i < command_table_size; i++) {
        const auto& entry = command_table[i];
        if (entry.menuName == menuName && entry.optionName == option) {
            if (entry.func) {
                entry.func(menuName, option, ansi);
            }
            return;
        }
    }
    
    // 2. 其次全局匹配（菜单名为空）
    for (int i = 0; i < command_table_size; i++) {
        const auto& entry = command_table[i];
        if (entry.menuName.empty() && entry.optionName == option) {
            if (entry.func) {
                entry.func(menuName, option, ansi);
            }
            return;
        }
    }

    // 3. 未找到匹配
    if (ansi) SetConsoleColor(BRIGHT_RED);
    cerr << "⚠ 未知命令: [" << menuName << "] " << option << endl;
    if (ansi) ResetConsoleColor();
    Sleep(1000);
}