#pragma once
#include <QMainWindow>
#include "article.h"

class QTabWidget;
class QListWidget;
class QCheckBox;
class ReaderView;
class PracticeView;
class ImportWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void openArticle(int index, bool resetPractice = true);
private slots:
    void refreshLibrary();
    void onLibraryOpen();
    void onImportOpen(int index);
    void onHintsToggled(bool on);
private:
    ArticleStore store_;
    QTabWidget*  tabs_;
    QListWidget* libList_;
    QCheckBox*   hintChk_;
    ReaderView*  reader_;
    PracticeView* practice_;
    ImportWidget* import_;
};
