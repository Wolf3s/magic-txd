#pragma once

#include <QDialog>

class MainWindow;

class CreateTxdDialog : public QDialog {
public:
    CreateTxdDialog(MainWindow *MainWnd);
    //~CreateTxdDialog(void);

    bool GetSelectedVersion(rw::LibraryVersion& verOut);

    void UpdateAccessibility(void);

public slots:
    void OnChangeVersion(const QString& newText);
    void OnChangeSelectedGame(int newIndex);
    void OnChangeSelecteedPlatform(int newIndex);
    void OnRequestAccept(bool clicked);
    void OnRequestCancel(bool clicked);
    void OnUpdateTxdName(const QString& newText);

    MainWindow *mainWnd;

    QLineEdit *txdName;
    QPushButton *applyButton;
    QLineEdit *versionLineEdit;
    QLineEdit *buildLineEdit;
    QComboBox *gameSelectBox;
    QComboBox *platSelectBox;
    QComboBox *dataTypeSelectBox;
};