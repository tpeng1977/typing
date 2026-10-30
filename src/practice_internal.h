#pragma once
#include <QString>

// PracticeView 内部多文件共享的常量与工具函数（仅编译单元内部使用）。

// 连续 5 秒无输入视为暂停
inline constexpr qint64 kPauseMs = 5000;
// 最近 5 分钟速度窗口（毫秒）
inline constexpr qint64 kWindowMs = 300000;

// HTML 转义（用于富文本提示区与右侧句子对照面板）
inline QString esc(const QString& s)
{
    QString out = s;
    out.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");
    return out;
}
