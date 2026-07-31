#ifndef SELECTOR_H
#define SELECTOR_H

#include <string>
#include <vector>
#include <windows.h>

using namespace std;

/**
 * 高性能控制台选择器
 * 特点：
 * - 局部刷新（只更新变化部分）
 * - 固定坐标绘制
 * - 支持滚动窗口
 * - 支持 ANSI/Windows 颜色
 */
class Selector {
private:
    // 配置
    struct Config {
        int x = 0;              // 起始 X 坐标
        int y = 0;              // 起始 Y 坐标
        int windowSize = 3;     // 窗口显示行数（不包括标题和指示器）
        bool ansi = false;      // 是否使用 ANSI 转义码
        int maxWidth = 80;      // 最大显示宽度
    } config_;

    // 数据
    std::vector<std::string> items_;  // items_[0] = 标题, items_[1..] = 选项
    int currentLine_ = 0;             // 当前高亮行（0-based，相对于选项列表）
    int windowStart_ = 0;             // 窗口起始索引
    int totalItems_ = 0;              // 总选项数（不含标题）
    
    // 缓存
    std::string cachedBuffer_;        // 缓存渲染内容
    
    // 私有方法
    void render();
    void clearArea();
    void gotoxy(int x, int y);
    void setColor(WORD color);
    void resetColor();
    void setCursorVisible(bool visible);
    int countOptions() const { return items_.size() - 2; }

public:
    // 构造函数
    Selector(const std::vector<std::string>& items, 
             int x = 0, int y = 0, 
             int windowSize = 3,
             bool ansi = false);
    
    // 运行选择器，返回选中索引（0-based），-1 表示取消
    int run();
    
    // 获取当前选中的文本
    std::string getSelectedText() const;
    
    // 获取当前选中的索引
    int getSelectedIndex() const { return currentLine_; }
    
    // 设置位置
    void setPosition(int x, int y) { config_.x = x; config_.y = y; }
    
    // 设置窗口大小
    void setWindowSize(int size) { config_.windowSize = size; }
};

// ===================== 便捷工具函数 =====================

/**
 * 快速选择器 - 从列表中选择一项
 * @param items 选项列表（第一个为标题）
 * @param x, y 起始坐标
 * @param windowSize 窗口大小
 * @return 选中索引（-1 表示取消）
 */
int quickSelect(const std::vector<std::string>& items, 
                int x = 0, int y = 0, 
                int windowSize = 3);

/**
 * 带描述的选项选择器
 * @param title 标题
 * @param options 选项列表
 * @param descriptions 描述列表（与选项一一对应）
 * @param x, y 起始坐标
 * @return 选中索引（-1 表示取消）
 */
int selectWithDescription(const std::string& title,
                          const std::vector<std::string>& options,
                          const std::vector<std::string>& descriptions,
                          int x = 0, int y = 0);

#endif // SELECTOR_H