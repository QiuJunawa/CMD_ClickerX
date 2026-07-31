#include "Selector.h"
#include "mytools.h"

#include <stdexcept>
using namespace std;

// ===================== 构造函数 =====================
Selector::Selector(const std::vector<std::string>& items, 
                   int x, int y, 
                   int windowSize,
                   bool ansi) 
    : items_(items), currentLine_(0), windowStart_(0) {
    
    if (items_.size() < 2) {
        throw std::runtime_error("至少需要 1 个标题 + 1 个选项");
    }
    
    config_.x = x;
    config_.y = y;
    config_.windowSize = std::max(1, windowSize);
    config_.ansi = ansi;
    config_.maxWidth = 80;
    
    totalItems_ = countOptions();
    
    // 初始化窗口起始位置（尽量居中）
    windowStart_ = std::max(0, currentLine_ - config_.windowSize / 2);
    if (windowStart_ + config_.windowSize > totalItems_) {
        windowStart_ = std::max(0, totalItems_ - config_.windowSize);
    }
}

// ===================== 渲染 =====================
void Selector::render() {
    std::string buffer = "";
    int x = config_.x;
    int y = config_.y;
    
    // 移动到起始位置
    gotoxy(x, y);
    
    // ===== 1. 显示标题 =====
    if (config_.ansi) {
        buffer += "\033[37m" + items_[1] + "\033[K\033[0m\n";
    } else {
        setColor(WHITE);
        buffer += items_[0];
        // 填充空格到最大宽度
        if ((int)items_[0].length() < config_.maxWidth) {
            buffer += std::string(config_.maxWidth - items_[0].length(), ' ');
        }
        buffer += '\n';
        resetColor();
    }
    
    // ===== 2. 顶部指示器 =====
    if (config_.ansi) {
        buffer += "\033[90m";
    } else {
        setColor(GRAY);
    }
    if (windowStart_ > 0) {
        buffer += "...";
    } else {
        buffer += "---";
    }
    buffer += std::string(config_.maxWidth - 3, ' ');
    buffer += '\n';
    if (!config_.ansi) resetColor();
    else buffer += "\033[0m";
    
    // ===== 3. 选项列表 =====
    int startIdx = windowStart_;
    int endIdx = std::min(windowStart_ + config_.windowSize, totalItems_);
    
    for (int i = startIdx; i < endIdx; i++) {
        // 实际选项索引 = i + 1（因为 items_[0] 是标题）
        int itemIdx = i + 2;
        if (i == currentLine_) {
            // 高亮项
            if (config_.ansi) {
                buffer += "\033[32m-> " + items_[itemIdx] + "\033[K\033[0m\n";
            } else {
                buffer += "-> " + items_[itemIdx];
                buffer += std::string(config_.maxWidth - items_[itemIdx].length() - 3, ' ');
                buffer += '\n';
            }
        } else {
            // 普通项
            if (config_.ansi) {
                buffer += "- " + items_[itemIdx] + "\033[K\n";
            } else {
                buffer += "- " + items_[itemIdx];
                buffer += std::string(config_.maxWidth - items_[itemIdx].length() - 2, ' ');
                buffer += '\n';
            }
        }
    }
    
    // ===== 4. 底部指示器 =====
    if (config_.ansi) {
        buffer += "\033[90m";
    } else {
        setColor(GRAY);
    }
    if (windowStart_ + config_.windowSize < totalItems_) {
        buffer += "...";
    } else {
        buffer += "---";
    }
    buffer += std::string(config_.maxWidth - 3, ' ');
    buffer += '\n';
    if (!config_.ansi) resetColor();
    
    // ===== 5. 输出 =====
    // 只在内容变化时输出
    if (buffer != cachedBuffer_) {
        gotoxy(x, y);
        std::cout << buffer << std::flush;
        cachedBuffer_ = buffer;
        if (config_.ansi)
            cout << "\033[0m";
    }
}

// ===================== 清除区域 =====================
void Selector::clearArea() {
    int totalLines = std::min(config_.windowSize + 3, totalItems_ + 2) + 1;
    
    gotoxy(config_.x, config_.y);
    for (int i = 0; i < totalLines; i++) {
        std::cout << std::string(config_.maxWidth, ' ');
        if (i < totalLines - 1) std::cout << '\n';
    }
    std::cout << std::flush;
}

// ===================== 运行选择器 =====================
int Selector::run() {
    COORD originalPos = getxy();
    setCursorVisible(false);
    cachedBuffer_.clear();

    // ---------- 设置控制台输入模式以接收鼠标事件 ----------
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD dwOriginalMode, dwMode;
    GetConsoleMode(hInput, &dwOriginalMode);
    dwMode = dwOriginalMode;
    // 禁用 QuickEdit 模式（会干扰鼠标事件）
    dwMode &= ~ENABLE_QUICK_EDIT_MODE;
    // 启用鼠标输入
    dwMode |= ENABLE_MOUSE_INPUT;
    // 确保扩展标志开启（Windows 10+ 可能需要）
    dwMode |= ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(hInput, dwMode);

    bool needRender = true;
    int result = -1;

    while (true) {
        if (needRender) {
            render();
            needRender = false;
        }

        // ---------- 读取输入事件 ----------
        DWORD cEvents;
        GetNumberOfConsoleInputEvents(hInput, &cEvents);
        if (cEvents == 0) {
            Sleep(20);
            continue;
        }

        INPUT_RECORD ir[8];
        DWORD read;
        ReadConsoleInput(hInput, ir, 8, &read);

        for (DWORD i = 0; i < read; i++) {
            // ---------- 处理键盘事件 ----------
            if (ir[i].EventType == KEY_EVENT && ir[i].Event.KeyEvent.bKeyDown) {
                WORD vk = ir[i].Event.KeyEvent.wVirtualKeyCode;
                // 忽略重复按键（按住时连续触发）
                if (ir[i].Event.KeyEvent.wRepeatCount > 1) continue;

                // 方向键
                if (vk == VK_UP) {
                    if (currentLine_ > 0) {
                        currentLine_--;
                        // 居中算法
                        windowStart_ = currentLine_ - config_.windowSize / 2;
                        if (windowStart_ < 0) windowStart_ = 0;
                        if (windowStart_ + config_.windowSize > totalItems_) {
                            windowStart_ = totalItems_ - config_.windowSize;
                        }
                        needRender = true;
                    }
                }
                else if (vk == VK_DOWN) {
                    if (currentLine_ < totalItems_ - 1) {
                        currentLine_++;
                        windowStart_ = currentLine_ - config_.windowSize / 2;
                        if (windowStart_ < 0) windowStart_ = 0;
                        if (windowStart_ + config_.windowSize > totalItems_) {
                            windowStart_ = totalItems_ - config_.windowSize;
                        }
                        needRender = true;
                    }
                }
                else if (vk == VK_RETURN) {
                    result = currentLine_;
                    // 清除输入缓冲区残留
                    FlushConsoleInputBuffer(hInput);
                    SetConsoleMode(hInput, dwOriginalMode);
                    // ... 退出循环
                    goto exit_loop;
                }
                else if (vk == VK_ESCAPE) {
                    result = -1;
                    FlushConsoleInputBuffer(hInput);
                    SetConsoleMode(hInput, dwOriginalMode);
                    goto exit_loop;
                }
                // 其他按键可忽略，也可添加 PageUp/PageDown 等
            }

            // ---------- 处理鼠标事件 ----------
            if (ir[i].EventType == MOUSE_EVENT) {
                MOUSE_EVENT_RECORD mer = ir[i].Event.MouseEvent;
                if (mer.dwEventFlags == MOUSE_WHEELED) {
                    short delta = HIWORD(mer.dwButtonState);
                    if (delta > 0) {    // 向上滚动
                        if (currentLine_ > 0) {
                            currentLine_--;
                            windowStart_ = currentLine_ - config_.windowSize / 2;
                            if (windowStart_ < 0) windowStart_ = 0;
                            if (windowStart_ + config_.windowSize > totalItems_) {
                                windowStart_ = totalItems_ - config_.windowSize;
                            }
                            needRender = true;
                        }
                    } else if (delta < 0) { // 向下滚动
                        if (currentLine_ < totalItems_ - 1) {
                            currentLine_++;
                            windowStart_ = currentLine_ - config_.windowSize / 2;
                            if (windowStart_ < 0) windowStart_ = 0;
                            if (windowStart_ + config_.windowSize > totalItems_) {
                                windowStart_ = totalItems_ - config_.windowSize;
                            }
                            needRender = true;
                        }
                    }
                }
            }
        }
    }

exit_loop:
    // ---------- 恢复原始输入模式 ----------
    SetConsoleMode(hInput, dwOriginalMode);

    clearArea();
    gotoxy(originalPos.X, originalPos.Y);
    setCursorVisible(true);
    return result;
}

// ===================== 获取选中文本 =====================
std::string Selector::getSelectedText() const {
    if (currentLine_ >= 0 && currentLine_ < totalItems_) {
        return items_[currentLine_ + 2];
    }
    return "";
}

// ===================== 工具函数实现 =====================

int quickSelect(const std::vector<std::string>& items, 
                int x, int y, 
                int windowSize) {
    try {
        Selector selector(items, x, y, windowSize, false);
        return selector.run();
    } catch (const std::exception& e) {
        std::cerr << "选择器错误: " << e.what() << std::endl;
        return -1;
    }
}

int selectWithDescription(const std::string& title,
                          const std::vector<std::string>& options,
                          const std::vector<std::string>& descriptions,
                          int x, int y) {
    // 构建选项列表
    std::vector<std::string> items;
    items.push_back(title);
    for (const auto& opt : options) {
        // 选项 + 描述（用括号括起来）
        items.push_back(opt + " (" + descriptions[items.size() - 1] + ")");
    }
    
    return quickSelect(items, x, y, 3);
}

// ===================== 私有方法实现 =====================

void Selector::gotoxy(int x, int y) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(hConsole, coord);
}

void Selector::setColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void Selector::resetColor() {
    setColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void Selector::setCursorVisible(bool visible) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hConsole, &cci);
    cci.bVisible = visible;
    SetConsoleCursorInfo(hConsole, &cci);
}