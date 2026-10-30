#pragma once
#include <QLineEdit>
#include <QString>

// 与 JustifiedLabel 严格对齐的输入框：
// 用整行原文计算词间距（英文两端对齐时与上方原文行逐词同位置），
// 绘制 [0, typedLen) 已输入部分 + 错误尾部 + IME 预编辑编码 + 光标。
class JustifiedInputLine : public QLineEdit {
    Q_OBJECT
public:
    explicit JustifiedInputLine(QWidget* parent = nullptr);
    // fullLine：整行原文；typedLen：已正确输入长度；
    // justified：该行是否两端对齐（与上方原文行一致）
    void setLineLayout(const QString& fullLine, int typedLen, bool justified);
    // 手动设置左侧偏移像素（替代样式表 padding，确保与 JustifiedLabel 严格对齐）
    void setLeftPad(int px) { leftPad_ = px; update(); }
    // 当前 IME 预编辑串（汉字上屏前的编码字母），供测试与调试
    QString preeditString() const { return preedit_; }
protected:
    void paintEvent(QPaintEvent*) override;
    void inputMethodEvent(QInputMethodEvent* e) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void focusOutEvent(QFocusEvent* e) override;
private:
    QString fullLine_;
    QString preedit_;          // 输入法组合中的编码（拼音/五笔字母等）
    int typedLen_ = 0;
    bool justified_ = false;
    int leftPad_ = 0;
    mutable int lastCursorX_ = 0;  // 供 ImCursorRectangle 定位候选框
};
