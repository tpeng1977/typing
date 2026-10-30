#include <QString>
#include <QVector>
#include <QStringList>
#include <cstdio>
#include "dataloader.h"
#include "bilingual.h"
#include "segmentation.h"
#include "article.h"
#include "buildinfo.h"

// 无界面自检：验证数据加载、分词、编码查询、双语数据是否正常
int runSelfTest()
{
    fprintf(stderr, "SELFTEST: 版本 %s · 编译于 %s %s\n",
            APP_VERSION, BUILD_DATE, BUILD_TIME);
    Dict& d = theDict();
    if (!d.ok()) {
        fprintf(stderr, "SELFTEST FAIL: 词典加载失败\n");
        return 1;
    }
    fprintf(stderr, "SELFTEST: 词典加载成功\n");
    // 抽查几个常见字
    const char* samples[] = {"五", "笔", "字", "型", "输", "入", "法"};
    bool bad = false;
    for (const char* s : samples) {
        const QString c = QString::fromUtf8(s);
        const QString code = d.wubiFull(c);
        const QString py = d.pinyin(c);
        fprintf(stderr, "  %s: 五笔=%s 拼音=%s\n", s, code.toUtf8().constData(),
                py.toUtf8().constData());
        if (code.isEmpty() || py.isEmpty()) bad = true;
    }
    if (bad) {
        fprintf(stderr, "SELFTEST FAIL: 存在缺失编码的字\n");
        return 1;
    }

    // 分词测试
    const QString text = QStringLiteral("人工智能正在深刻改变人类社会的运行方式，从语音助手到自动驾驶，再到金融风险控制。");
    QVector<Token> toks = segment(text, d);
    QStringList words;
    for (const Token& t : toks) {
        if (t.isWord) words << text.mid(t.start, t.len);
    }
    fprintf(stderr, "SELFTEST: 分词得到 %d 个词: %s\n", int(words.size()),
            words.join(" / ").toUtf8().constData());

    // 回归：常见词与领域词必须被切分为词组（曾因词表截断/缺失而漏分）
    const QString t2 = QStringLiteral("按照年初制定的发展计划，我们应用新技术开拓市场，人工智能与自动驾驶正在改变金融科技的方方面面。");
    QVector<Token> toks2 = segment(t2, d);
    QStringList words2;
    for (const Token& t : toks2)
        if (t.isWord) words2 << t2.mid(t.start, t.len);
    const char* must[] = {"计划", "应用", "市场", "人工智能", "自动驾驶", "金融科技"};
    for (const char* w : must) {
        const QString q = QString::fromUtf8(w);
        if (!words2.contains(q)) {
            fprintf(stderr, "SELFTEST FAIL: 「%s」未被切分为词组 (分词: %s)\n", w,
                    words2.join(" / ").toUtf8().constData());
            return 1;
        }
    }
    fprintf(stderr, "SELFTEST: 常见词/领域词回归检查通过（计划/应用/市场/人工智能/自动驾驶/金融科技）\n");

    // 词语编码
    const QString w = QStringLiteral("人工智能");
    fprintf(stderr, "SELFTEST: 词语「人工智能」编码 = %s\n", d.wordCode(w).toUtf8().constData());

    // 文章库
    ArticleStore store;
    store.load();
    fprintf(stderr, "SELFTEST: 内置文章 %d 篇\n", store.count());
    for (int i = 0; i < store.count(); ++i) {
        const Article a = store.at(i);
        fprintf(stderr, "  [%02d] %s  %d 字\n", i + 1, a.title.toUtf8().constData(),
                int(a.body.size()));
    }
    if (store.count() < 20) {
        fprintf(stderr, "SELFTEST FAIL: 内置文章少于 20 篇\n");
        return 1;
    }

    // 双语数据：20 篇逐句中英对照 + 英文释义词典
    BilingualStore& b = theBilingual();
    if (!b.ok()) {
        fprintf(stderr, "SELFTEST FAIL: 双语数据加载失败\n");
        return 1;
    }
    fprintf(stderr, "SELFTEST: 双语数据加载成功（%d 篇译文，%d 词释义）\n",
            b.articleCount(), b.wordCount());
    int totalPairs = 0;
    for (int i = 0; i < store.count(); ++i) {
        const int n = b.sentences(store.at(i).title).size();
        totalPairs += n;
        if (n == 0) {
            fprintf(stderr, "SELFTEST FAIL: 《%s》缺少译文\n",
                    store.at(i).title.toUtf8().constData());
            return 1;
        }
    }
    fprintf(stderr, "SELFTEST: 逐句中英对照共 %d 句\n", totalPairs);
    const char* enSamples[] = {"artificial", "blockchain", "money", "principle", "stages"};
    for (const char* w : enSamples) {
        const QString q = QString::fromUtf8(w);
        if (b.meaning(q).isEmpty()) {
            fprintf(stderr, "SELFTEST FAIL: 单词「%s」缺少释义\n", w);
            return 1;
        }
    }
    fprintf(stderr, "SELFTEST: 英文释义抽查通过（artificial/blockchain/money/principle/stages）\n");

    fprintf(stderr, "SELFTEST OK\n");
    return 0;
}
