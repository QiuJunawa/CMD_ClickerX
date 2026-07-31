#ifndef RUNNER_H
#define RUNNER_H

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <windows.h>
#include <indicators.hpp>
#include "mytools.h"

using namespace std;

// runner.h 全局热键变量（所有cpp共享）
extern int g_hotkeyId;
extern int g_hotkeyVkCode;


// 命令条目结构
struct CommandEntry {
    string menuName;
    string optionName;
    std::function<void(const string&, const string&, bool)> func;
};

// 热键管理类
class HotkeyManager {
private:
    struct HotkeyEntry {
        int vkCode;                           // 虚拟键码，如 VK_F1
        std::function<void()> callback;       // 触发时执行的回调
        bool isPressed = false;               // 当前是否按下（用于防止重复触发）
        // 手动三参数构造函数
        HotkeyEntry(int vk, std::function<void()> cb, bool pressed)
            : vkCode(vk), callback(std::move(cb)), isPressed(pressed)
        {}
    };

    std::unordered_map<int, HotkeyEntry> hotkeys_;  // key: 唯一ID
    std::atomic<bool> running_{false};
    std::thread workerThread_;

    void workerLoop() {
        while (running_) {
            for (auto& pair : hotkeys_) {
                HotkeyEntry& entry = pair.second;
                // 完整判断：bit15=当前按下，bit0=本次循环内有按下动作
                SHORT keyState = GetAsyncKeyState(entry.vkCode);
                bool nowPressed = (keyState & 0x8000) != 0;
                bool keyTriggered = (keyState & 0x0001) != 0;

                // 按下瞬间触发（边沿触发，防长按重复）
                if (nowPressed && !entry.isPressed && keyTriggered) {
                    if (entry.callback) {
                        // 异步执行回调，避免阻塞热键轮询
                        std::thread cbThread(entry.callback);
                        cbThread.detach();
                    }
                }
                entry.isPressed = nowPressed;
            }
            // 20ms 轮询，不占用CPU，后台依然稳定读取键盘快照
            Sleep(20);
        }
    }

public:
    // 启动热键监听
    void start() {
        if (running_) return;
        running_ = true;
        workerThread_ = std::thread(&HotkeyManager::workerLoop, this);
    }

    // 停止监听（析构时会自动调用）
    void stop() {
        if (running_) {
            running_ = false;
            if (workerThread_.joinable()) {
                workerThread_.join();
            }
        }
    }

    // 注册热键，返回ID用于后续修改
    int registerHotkey(int vkCode, std::function<void()> callback) {
        static int nextId = 1;
        int id = nextId++;
        hotkeys_.emplace(id, HotkeyEntry(vkCode, callback, false));
        return id;
    }

    // 修改热键的虚拟键码
    void modifyHotkey(int id, int newVkCode) {
        auto it = hotkeys_.find(id);
        if (it != hotkeys_.end()) {
            it->second.vkCode = newVkCode;
        }
    }

    // 修改热键的回调
    void modifyCallback(int id, std::function<void()> newCallback) {
        auto it = hotkeys_.find(id);
        if (it != hotkeys_.end()) {
            it->second.callback = newCallback;
        }
    }

    // 取消注册
    void unregisterHotkey(int id) {
        hotkeys_.erase(id);
    }

    void resetAllHotkeys()
    {
        stop();
        hotkeys_.clear();
    }

    ~HotkeyManager() {
        stop();
    }
};

// 执行命令（原有函数）
void execute_command(const string& menuName, const string& option, bool ansi);

extern CommandEntry command_table[];
extern int command_table_size;

#endif // RUNNER_H