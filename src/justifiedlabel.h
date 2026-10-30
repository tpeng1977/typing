#pragma once
#include <QLabel>

class QPaintEvent;

// 支持 Word 式两端对齐（justify）的行标签：
// 非最后一行且行文本含空格时，自动拉伸词间空格使左右边缘对齐；
// 当前行支持分段着色（已输入蓝色、目标黄底加粗、其余默认色）。
class JustifiedLabel : public QLabel {
    Q_OBJECT
public:
    explicit JustifiedLabel(const QString& color, int height, QWidget* parent = nullptr);
    // 当前行着色：已输入 [0,start) 蓝色、目标 [start,start+len) 黄底加粗、其余默认色；
    // start < 0 表示整行单一颜色（普通行）
    void setTarget(int start, int len);
    // 文章最后一行不两端对齐（Word 规则）
    void setLastRow(bool last);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void drawWord(QPainter& p, const QRect& r, int baseline, int x,
                  const QString& text, int ws, int we);
    QString color_;
    int targetStart_ = -1;
    int targetLen_ = 0;
    bool lastRow_ = false;
};
