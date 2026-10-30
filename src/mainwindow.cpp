#include "mainwindow.h"
#include "reader.h"
#include "practice.h"
#include "importwidget.h"
#include "buildinfo.h"
#include <QTabWidget>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QStatusBar>
#include <QSettings>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    store_.load();

    setWindowTitle(QStringLiteral("打字练习 %1").arg(APP_VERSION_ZH));
    resize(1180, 780);
    // 允许横向缩小；具体下限由布局/练习区可缩标签保证
    setMinimumSize(720, 480);

    // ---- 文章库 ----
    libList_ = new QListWidget(this);
    libList_->setStyleSheet("QListWidget{font-size:14px;}");
    QPushButton* openBtn = new QPushButton(QStringLiteral("打开所选文章"), this);
    QPushButton* practiceBtn = new QPushButton(QStringLiteral("开始练习"), this);
    openBtn->setStyleSheet("QPushButton{background:#1d4ed8;color:#fff;border:none;border-radius:6px;"
                           "padding:8px 14px;font-weight:bold;}");
    practiceBtn->setStyleSheet("QPushButton{background:#0f766e;color:#fff;border:none;border-radius:6px;"
                               "padding:8px 14px;font-weight:bold;}");
    QHBoxLayout* libBtns = new QHBoxLayout;
    libBtns->addWidget(openBtn);
    libBtns->addWidget(practiceBtn);
    libBtns->addStretch();

    QGroupBox* libBox = new QGroupBox(QStringLiteral("文章库（双击打开）"), this);
    QVBoxLayout* libLay = new QVBoxLayout(libBox);
    libLay->addWidget(libList_, 1);
    libLay->addLayout(libBtns);

    // ---- 各标签页 ----
    reader_ = new ReaderView(this);
    practice_ = new PracticeView(this);
    import_ = new ImportWidget(&store_, this);

    tabs_ = new QTabWidget(this);
    tabs_->addTab(libBox, QStringLiteral("文章库"));
    tabs_->addTab(reader_, QStringLiteral("阅读 · 提示"));
    tabs_->addTab(practice_, QStringLiteral("练习"));
    tabs_->addTab(import_, QStringLiteral("导入文章"));

    // 顶部：五笔/拼音编码提示总开关（可手动关闭，设置持久化）
    hintChk_ = new QCheckBox(QStringLiteral("显示编码提示（五笔 / 拼音）"), this);
    hintChk_->setChecked(true);
    QHBoxLayout* topRow = new QHBoxLayout;
    topRow->addWidget(hintChk_);
    topRow->addStretch();

    QWidget* central = new QWidget(this);
    QVBoxLayout* cl = new QVBoxLayout(central);
    cl->setContentsMargins(6, 4, 6, 0);
    cl->addLayout(topRow);
    cl->addWidget(tabs_, 1);
    setCentralWidget(central);

    // 状态栏长文案会抬高窗口最小宽度，导致只能拉大不能缩小；改短提示 + tooltip
    statusBar()->setSizeGripEnabled(true);
    statusBar()->showMessage(QStringLiteral("提示：悬停/点击可查看编码；练习页用系统输入法跟打。"));
    statusBar()->setToolTip(QStringLiteral(
        "阅读页：把鼠标移到字/词上可查看五笔编码与拼音；点击查看详情。\n"
        "练习页：用系统输入法（拼音/五笔）输入当前字词，自动前进。\n"
        "顶部「显示编码提示」可随时关闭所有提示。"));

    // 常驻右侧：代码版本与编译时间
    QLabel* verLabel = new QLabel(
        QStringLiteral("v%1 · 编译于 %2 %3")
            .arg(QString::fromUtf8(APP_VERSION),
                 QString::fromUtf8(BUILD_DATE),
                 QString::fromUtf8(BUILD_TIME)),
        this);
    verLabel->setStyleSheet("color:#64748b;font-size:12px;");
    statusBar()->addPermanentWidget(verLabel);

    connect(hintChk_, &QCheckBox::toggled, this, &MainWindow::onHintsToggled);
    connect(libList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { onLibraryOpen(); });
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::onLibraryOpen);
    connect(practiceBtn, &QPushButton::clicked, this, [this]{
        const int row = libList_->currentRow();
        if (row < 0) return;
        openArticle(row, true);          // 开始练习：加载文章并重置进度
        tabs_->setCurrentWidget(practice_);
    });
    connect(import_, &ImportWidget::requestOpen, this, &MainWindow::onImportOpen);

    // 恢复上次的提示开关状态并应用到各页面
    QSettings st;
    const bool hints = st.value("hintsEnabled", true).toBool();
    hintChk_->setChecked(hints);
    reader_->setHintsEnabled(hints);
    practice_->setHintsEnabled(hints);

    refreshLibrary();
    if (store_.count() > 0)
        openArticle(0);
}

void MainWindow::onHintsToggled(bool on)
{
    reader_->setHintsEnabled(on);
    practice_->setHintsEnabled(on);
    QSettings().setValue("hintsEnabled", on);
}

void MainWindow::refreshLibrary()
{
    libList_->clear();
    const int n = store_.count();
    for (int i = 0; i < n; ++i) {
        const Article a = store_.at(i);
        QString label = QStringLiteral("%1  %2  （%3 字）")
                            .arg(a.builtin ? QStringLiteral("[内置]") : QStringLiteral("[导入]"),
                                 a.title).arg(a.body.size());
        QListWidgetItem* it = new QListWidgetItem(label, libList_);
        it->setData(Qt::UserRole, i);
    }
}

void MainWindow::onLibraryOpen()
{
    const int row = libList_->currentRow();
    if (row < 0) return;
    const int idx = libList_->item(row)->data(Qt::UserRole).toInt();
    openArticle(idx, false);             // 阅读打开：不重置练习进度
    tabs_->setCurrentWidget(reader_);
}

void MainWindow::onImportOpen(int index)
{
    if (index < 0 || index >= store_.count()) return;
    openArticle(index, false);           // 阅读打开：不重置练习进度
    tabs_->setCurrentWidget(reader_);
}

void MainWindow::openArticle(int index, bool resetPractice)
{
    if (index < 0 || index >= store_.count()) return;
    const Article a = store_.at(index);
    reader_->setArticle(a.title, a.body);
    if (resetPractice)
        practice_->setArticle(a.title, a.body);
    setWindowTitle(QStringLiteral("打字练习 %1 — %2").arg(APP_VERSION_ZH, a.title));
}
