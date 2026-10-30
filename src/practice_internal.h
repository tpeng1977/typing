#pragma once
#include <QString>

// PracticeView 内部多文件共享的常量与工具函数（仅编译单元内部使用）。

// 连续 5 秒无输入视为暂停
inline constexpr qint64 kPauseMs = 5000;
// 最近 5 分钟速度窗口（毫秒）
inline constexpr qint64 kWindowMs = 300000;

// 练习行左右内边距（与 stylesheet padding 一致；右侧必须足够，否则 ;,. 等尾字符被裁切）
inline constexpr int kLinePadLeft = 0;
inline constexpr int kLinePadRight = 18;
// 折行额外安全边距：卡片边框 + 字体 right-bearing（分号等常超出 advance）
inline constexpr int kWrapSafety = 8;

// HTML 转义（用于富文本提示区与右侧句子对照面板）
inline QString esc(const QString& s)
{
    QString out = s;
    out.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");
    return out;
}
