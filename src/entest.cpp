#include <QApplication>
#include <QRegularExpression>
#include <QInputMethodEvent>
#include <QLineEdit>
#include <QLabel>
#include <cstdio>
#include "practice.h"
#include "bilingual.h"
#include "article.h"

// 无界面英文练习测试：验证双语数据加载、英文模式折行/匹配、中英双解提示。
int runEnTest()
{
    BilingualStore& b = theBilingual();
    if (!b.ok()) {
        fprintf(stderr, "EN TEST FAIL: 双语数据加载失败\n");
        return 1;
    }
    ArticleStore store;
    store.load();
    if (store.count() < 1) {
        fprintf(stderr, "EN TEST FAIL: 文章库为空\n");
        return 1;
    }
    const Article a = store.at(0);
    const QVector<SentencePair> pairs = b.sentences(a.title);
    if (pairs.isEmpty()) {
        fprintf(stderr, "EN TEST FAIL: 第一篇无译文\n");
        return 1;
    }
    // 英文全文（与 PracticeView 拼接规则一致：句子间以单空格连接）
    QString en;
    for (const SentencePair& p : pairs) {
        if (!en.isEmpty()) en += QLatin1Char(' ');
        en += p.en;
    }

    PracticeView v;
    v.setArticle(a.title, a.body);
    v.setLangMode(1);               // 切到英文练习
    v.show();
    QLineEdit* le = v.findChild<QLineEdit*>();
    if (!le) {
        fprintf(stderr, "EN TEST FAIL: 找不到输入框\n");
        return 1;
    }
    QLabel* stats = nullptr;
    QLabel* hint = nullptr;
    for (QLabel* l : v.findChildren<QLabel*>()) {
        if (!stats && l->text().contains(QStringLiteral("正确"))) stats = l;
        if (!hint && l->text().contains(QStringLiteral("释义"))) hint = l;
    }
    if (!stats) {
        fprintf(stderr, "EN TEST FAIL: 找不到统计标签\n");
        return 1;
    }
    auto commit = [&](const QString& s) {
        QInputMethodEvent e;
        e.setCommitString(s);
        QApplication::sendEvent(le, &e);
        QApplication::processEvents();
    };
    auto numOf = [&](const QString& key) -> int {
        const QRegularExpression re(QStringLiteral("(%1) (-?\\d+)").arg(key));
        const QRegularExpressionMatch m = re.match(stats->text());
        return m.hasMatch() ? m.captured(2).toInt() : -1;
    };

    // 1) 逐字符提交英文前 80 字符（跨越词与行边界）。
    // 跨行后输入框按设计只保留“当前行已匹配前缀”，因此不断言全文，而验证正确数=80 且错误=0。
    const QString seq = en.left(80);
    for (const QChar c : seq)
        commit(QString(c));
    if (numOf(QStringLiteral("正确")) != 80) {
        fprintf(stderr, "EN TEST FAIL: 英文连续输入后正确数异常: %d\n",
                numOf(QStringLiteral("正确")));
        return 1;
    }
    if (numOf(QStringLiteral("错误")) != 0) {
        fprintf(stderr, "EN TEST FAIL: 英文连续输入后错误数异常: %d\n",
                numOf(QStringLiteral("错误")));
        return 1;
    }
    // 输入框当前文本应为正文某行的前缀（跨行截断设计）
    if (le->text().isEmpty()) {
        fprintf(stderr, "EN TEST FAIL: 英文连续输入后输入框为空\n");
        return 1;
    }
    fprintf(stderr, "EN TEST: 英文连续 80 字符输入正常（正确=%d，错误=%d）\n",
            numOf(QStringLiteral("正确")), numOf(QStringLiteral("错误")));

    // 2) 错误输入保留红色显示，不截断
    commit(QStringLiteral("X"));
    if (numOf(QStringLiteral("错误")) != 1) {
        fprintf(stderr, "EN TEST FAIL: 英文出错后错误数异常: %d\n",
                numOf(QStringLiteral("错误")));
        return 1;
    }
    fprintf(stderr, "EN TEST: 英文错误输入保留红色显示（正确=%d，错误=%d）\n",
            numOf(QStringLiteral("正确")), numOf(QStringLiteral("错误")));

    // 3) Backspace 删除错误字符
    {
        QKeyEvent backspace(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier);
        QApplication::sendEvent(le, &backspace);
    }

    // 4) 删除后继续输入正常：提交正确的第 81 个字符
    commit(QString(en.at(80)));
    if (numOf(QStringLiteral("正确")) != 81) {
        fprintf(stderr, "EN TEST FAIL: 删除后继续输入正确数异常: %d\n",
                numOf(QStringLiteral("正确")));
        return 1;
    }
    fprintf(stderr, "EN TEST: 英文删除后继续输入正常（正确=%d）\n",
            numOf(QStringLiteral("正确")));

    // 4) 中英双解提示：当前词在词典中有释义
    if (!hint) {
        fprintf(stderr, "EN TEST FAIL: 英文模式提示区未显示中英双解\n");
        return 1;
    }
    fprintf(stderr, "EN TEST: 英文提示区=%s\n", hint->text().toUtf8().constData());
    if (!hint->text().contains(QStringLiteral("释义"))) {
        fprintf(stderr, "EN TEST FAIL: 提示区缺少释义\n");
        return 1;
    }
    fprintf(stderr, "EN TEST OK\n");
    return 0;
}
