#include <QApplication>
#include <QFont>
#include <QIcon>
#include <cstdio>
#include "mainwindow.h"
#include "buildinfo.h"

// 无界面自检（定义于 selftest.cpp / imetest.cpp / entest.cpp）
int runSelfTest();
int runImeTest();
int runEnTest();

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("typing"));
    app.setOrganizationName(QStringLiteral("typing"));
    app.setApplicationDisplayName(QStringLiteral("打字练习"));

    if (app.arguments().contains(QStringLiteral("--selftest"))) {
        return runSelfTest();
    }
    if (app.arguments().contains(QStringLiteral("--ime-test"))) {
        return runImeTest();
    }
    if (app.arguments().contains(QStringLiteral("--en-test"))) {
        return runEnTest();
    }

    QFont font(QStringLiteral("Microsoft YaHei"));
    font.setPointSize(11);
    app.setFont(font);

    // 应用图标：五笔字根 + 键盘主题
    app.setWindowIcon(QIcon(QStringLiteral(":/app_icon.png")));

    MainWindow w;
    w.show();
    return app.exec();
}
