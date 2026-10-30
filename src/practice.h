#pragma once
#include <QWidget>
#include <QElapsedTimer>
#include "segmentation.h"
#include "bilingual.h"

class QLabel;
class JustifiedLabel;
class JustifiedInputLine;
class QComboBox;
class QPushButton;
class QTimer;
class QFrame;

// 跟打式练习视图：
// 界面上方固定显示当前词的注音（拼音）与五笔编码提示（英文模式显示单词中英双解）；
// 下方为整齐的原文区——已完成行以“原文+输入”成对展示（置灰），
// 当前行原文在上、输入框紧贴其下方（同字号对齐、输入内容逐字累积），后续行灰蓝待输入。
// 右侧为句子对照面板：练中文时高亮当前输入内容对应的英文句子，
// 练英文时高亮当前输入英文句子对应的中文句子。
// 统计在窗体上部；连续 5 秒无输入视为暂停，暂停期间不计入用时与速度。
class PracticeView : public QWidget {
    Q_OBJECT
public:
    explicit PracticeView(QWidget* parent = nullptr);
    void setArticle(const QString& title, const QString& body);
    void setHintsEnabled(bool on);   // 关闭后隐藏注音/编码提示
    void setLangMode(int mode);      // 0=中文练习, 1=英文练习（供测试与切换）
    int  langMode() const { return langMode_; }
protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;   // 捕获按键/输入法输入活动
    void resizeEvent(QResizeEvent* ev) override;           // 宽度变化时按新宽度重排并保留进度
private slots:
    void onTextEdited();
    void onRestart();
    void onSkip();
    void onModeChanged();
    void onLangChanged();
    void onTick();
private:
    void prepare();
    void buildLines();             // Word 式换行：整段按可用宽度自然折行，段首行缩进两字符
    void wrapEnglish();            // 英文按“词”折行，避免词中间断行
    void refresh();
    void updateStats();
    void updateRightPanel();       // 右侧句子对照面板
    void noteActivity();
    void processText();
    void advanceLine();
    int  absPos() const;             // 当前已正确输入的总字数（绝对位置）
    int  targetStartAbs() const;     // 当前目标（字/词）的绝对起始下标
    int  textAvailWidth() const;     // 当前行卡片内文字可用宽度
    int  charWidth() const;          // 文章字号下单个汉字的像素宽
    QString targetText() const;      // 当前目标（单字/词；英文模式为当前单词）
    QString targetCode() const;      // 目标编码（中文五笔；英文模式为空）
    QString targetPinyin() const;    // 目标注音（拼音；英文模式为空）
    QString targetMeaning() const;   // 英文模式下当前单词的中文释义
    QString currentEnWord() const;   // 英文模式下当前输入位置所在的单词
    int  targetStartInLine() const;  // 当前目标在当前行内的起始下标
    int  targetLenInLine() const;    // 当前目标在当前行内的长度
    int  currentZhSentence() const;  // 当前输入位置所在中文句（pairs_ 下标），无则 -1
    int  currentEnSentence() const;  // 当前输入位置所在英文句（pairs_ 下标），无则 -1

    struct Sample { qint64 wall; qint64 active; int correct; };  // 每秒速度采样

    QString title_, body_;           // body_ 为当前练习语言下的正文
    QString zhBody_;                 // 中文正文（练习英文时保留，用于切换回中文）
    QString enText_;                 // 英文译文全文（练习英文时作为 body_）
    QVector<int> enStarts_;          // 每个英文句在 enText_ 中的起始偏移
    QVector<SentencePair> pairs_;    // 当前文章的中英逐句对照
    QVector<Token> tokens_;
    QVector<QString> lines_;       // 按宽度自然折行的练习行（不含缩进字符）
    QVector<int> lineStarts_;      // 每行在正文中的绝对起始下标
    QVector<bool> indents_;        // 每行是否为段落首行（显示时缩进两字符）
    int lastBuildWidth_ = -1;      // 上次建行时的卡片宽度
    int lineIdx_ = 0;              // 当前练习行
    int typedLen_ = 0;             // 当前行已正确匹配的字符数
    bool wordMode_ = true;         // true=词语提示, false=单字提示
    int langMode_ = 0;             // 0=中文练习, 1=英文练习
    int correct_ = 0, error_ = 0, skip_ = 0;
    bool finished_ = false;
    bool hintsOn_ = true;

    // 计时与暂停
    QElapsedTimer clock_;         // 全程计时（相对毫秒）
    qint64 lastActivityMs_ = 0;   // 上次活动时刻
    qint64 activeMs_ = 0;         // 累计活动时间（不含暂停）
    bool   paused_ = false;       // 连续 5 秒无输入 → 暂停
    bool   compositionActive_ = false;  // 输入法预编辑中（仅用于避免缩放重排打断 IME）
    bool   suppressEdit_ = false; // 程序性改写输入框时抑制 textEdited 重入
    bool   processQueued_ = false; // 已排队一次延迟匹配（合并同帧多次提交）
    int    lastInputIndent_ = -1; // 输入框左缩进缓存，避免每键重设 stylesheet
    int    lastRenderLine_ = -1;  // 上次渲染的行号；同行内输入时跳过行结构重绘
    QTimer* idleTimer_ = nullptr; // 1 秒心跳：暂停检测 + 速度采样
    QVector<Sample> samples_;     // 速度窗口采样
    void scheduleProcessText();  // 延迟到当前 IME/按键事件结束后再匹配

    QLabel *articleLabel_, *hintLabel_, *statsLabel_;
    JustifiedLabel *done2Line_, *done2In_, *done1Line_, *done1In_;  // 已完成行（原文+输入成对）
    JustifiedLabel *lineLabel_, *up1_, *up2_, *up3_, *up4_, *up5_;  // 当前行原文 + 后续行
    QFrame* lineCard_;           // “当前行”卡片：原文在上、输入框在下
    JustifiedInputLine* input_;
    QComboBox* mode_, *langCmb_;
    QPushButton *restartBtn_, *skipBtn_;
    QFrame* rightCard_;          // 右侧句子对照面板
    QLabel *rightTitle_, *rightLabel_;
};
