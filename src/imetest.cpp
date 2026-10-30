#include <QApplication>
#include <QRegularExpression>
#include <QInputMethodEvent>
#include <QLineEdit>
#include <QLabel>
#include <cstdio>
#include "practice.h"
#include "dataloader.h"
#include "justifiedinput.h"

// 无界面 IME 链路测试：用 QInputMethodEvent 逐字模拟真实输入法提交，
// 验证连续输入不被吞（0.1.8 匹配起点错位导致逐字输入第二个字起失败）。
int runImeTest()
{
    Dict& d = theDict();
    if (!d.ok()) {
        fprintf(stderr, "IME TEST FAIL: 词典加载失败\n");
        return 1;
    }
    const QString body = QStringLiteral(
        "人工智能是一门让机器模仿人类思考、判断与学习的学问。"
        "这门学科诞生于二十世纪五十年代的一次夏季研讨会上，"
        "学者们首次用这个词称呼它，并乐观预言，一代人之内，"
        "机器就能完成人类所能做的一切智力工作。");
    PracticeView v;
    v.setArticle(QStringLiteral("IME测试"), body);
    v.show();
    QLineEdit* le = v.findChild<QLineEdit*>();
    if (!le) {
        fprintf(stderr, "IME TEST FAIL: 找不到输入框\n");
        return 1;
    }
    QLabel* stats = nullptr;
    for (QLabel* l : v.findChildren<QLabel*>()) {
        if (l->text().contains(QStringLiteral("正确"))) { stats = l; break; }
    }
    if (!stats) {
        fprintf(stderr, "IME TEST FAIL: 找不到统计标签\n");
        return 1;
    }
    auto* jle = qobject_cast<JustifiedInputLine*>(le);

    // 0) 预编辑编码必须可见：发送拼音/五笔字母 preedit 后控件应保留该串
    {
        QInputMethodEvent pre(QStringLiteral("wwww"), {});
        QApplication::sendEvent(le, &pre);
        QApplication::processEvents();
        if (!jle || jle->preeditString() != QStringLiteral("wwww")) {
            fprintf(stderr, "IME TEST FAIL: 预编辑编码未保留显示: '%s'\n",
                    jle ? jle->preeditString().toUtf8().constData() : "(null)");
            return 1;
        }
        fprintf(stderr, "IME TEST: 预编辑编码可见（preedit=%s）\n",
                jle->preeditString().toUtf8().constData());
        // 清空预编辑，避免影响后续提交测试
        QInputMethodEvent clearPre;
        QApplication::sendEvent(le, &clearPre);
        QApplication::processEvents();
    }

    // 与真实输入法提交相同的 Qt 事件路径：commitString 插入到光标处。
    // 先发非空 preedit 再 commit，覆盖“组合态未清就 textEdited”的竞态。
    auto commit = [&](const QString& s) {
        QInputMethodEvent pre(QStringLiteral("…"), {});
        QApplication::sendEvent(le, &pre);
        QInputMethodEvent e;
        e.setCommitString(s);   // preedit 默认空 → 组合结束
        QApplication::sendEvent(le, &e);
        QApplication::processEvents(); // 吃掉延迟匹配定时器
    };
    auto numOf = [&](const QString& key) -> int {
        const QRegularExpression re(QStringLiteral("(%1) (-?\\d+)").arg(key));
        const QRegularExpressionMatch m = re.match(stats->text());
        return m.hasMatch() ? m.captured(2).toInt() : -1;
    };

    // 1) 连续逐字提交 7 个字（0.1.8 从此场景第 2 个字起匹配失败）
    const QString seq = QStringLiteral("人工智能是一门");
    for (const QChar c : seq)
        commit(QString(c));
    if (le->text() != seq) {
        fprintf(stderr, "IME TEST FAIL: 连续输入后输入框文本异常: '%s'\n",
                le->text().toUtf8().constData());
        return 1;
    }
    if (numOf(QStringLiteral("正确")) != 7) {
        fprintf(stderr, "IME TEST FAIL: 连续输入后正确数异常: %d\n",
                numOf(QStringLiteral("正确")));
        return 1;
    }
    fprintf(stderr, "IME TEST: 连续 7 字输入正常（输入框=%s，正确=%d，错误=%d）\n",
            le->text().toUtf8().constData(),
            numOf(QStringLiteral("正确")), numOf(QStringLiteral("错误")));

    // 2) 错误输入保留红色显示，不截断
    commit(QStringLiteral("X"));
    if (le->text() != seq + QStringLiteral("X")) {
        fprintf(stderr, "IME TEST FAIL: 错误输入未保留显示: '%s'\n",
                le->text().toUtf8().constData());
        return 1;
    }
    if (numOf(QStringLiteral("错误")) != 1) {
        fprintf(stderr, "IME TEST FAIL: 错误数异常: %d\n",
                numOf(QStringLiteral("错误")));
        return 1;
    }
    fprintf(stderr, "IME TEST: 错误输入保留红色显示（正确=%d，错误=%d，输入框=%s）\n",
            numOf(QStringLiteral("正确")), numOf(QStringLiteral("错误")),
            le->text().toUtf8().constData());

    // 3) Backspace 删除错误字符
    QKeyEvent backspace(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
    QApplication::sendEvent(le, &backspace);
    if (le->text() != seq) {
        fprintf(stderr, "IME TEST FAIL: Backspace 删除错误字符失败: '%s'\n",
                le->text().toUtf8().constData());
        return 1;
    }
    fprintf(stderr, "IME TEST: Backspace 删除错误字符正常（输入框=%s）\n",
            le->text().toUtf8().constData());

    // 4) 删除后继续输入正常
    commit(QStringLiteral("让"));
    if (le->text() != QStringLiteral("人工智能是一门让")) {
        fprintf(stderr, "IME TEST FAIL: 删除后继续输入失败: '%s'\n",
                le->text().toUtf8().constData());
        return 1;
    }
    if (numOf(QStringLiteral("正确")) != 8) {
        fprintf(stderr, "IME TEST FAIL: 删除后继续输入正确数异常: %d\n",
                numOf(QStringLiteral("正确")));
        return 1;
    }
    fprintf(stderr, "IME TEST: 删除后继续输入正常（正确=%d，输入框=%s）\n",
            numOf(QStringLiteral("正确")), le->text().toUtf8().constData());

    // 5) 标点符号可输入并计入正确（正文中的顿号/逗号等）
    // 当前位置后原文：机器模仿… —— 先把“机器模仿人类思考”打完再测顿号“、”
    const QString more = QStringLiteral("机器模仿人类思考");
    for (const QChar c : more)
        commit(QString(c));
    commit(QStringLiteral("、"));
    if (numOf(QStringLiteral("错误")) != 0) {
        fprintf(stderr, "IME TEST FAIL: 标点输入后错误数异常: %d（输入框=%s）\n",
                numOf(QStringLiteral("错误")), le->text().toUtf8().constData());
        return 1;
    }
    if (numOf(QStringLiteral("正确")) != 8 + more.size() + 1) {
        fprintf(stderr, "IME TEST FAIL: 标点输入后正确数异常: %d\n",
                numOf(QStringLiteral("正确")));
        return 1;
    }
    fprintf(stderr, "IME TEST: 中文标点输入正常（正确=%d）\n",
            numOf(QStringLiteral("正确")));
    fprintf(stderr, "IME TEST OK\n");
    return 0;
}
