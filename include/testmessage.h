#pragma once

#include <QMessageBox>
#include <QString>

template <typename... ArgTypes> void TestMessage(char *format, ArgTypes... Args) {
    char text[512];
    sprintf_s(text, format, Args...);
    QMessageBox msgBox;
    msgBox.setWindowTitle("Test message");
    msgBox.setText(text);
    msgBox.exec();
}

template <typename... ArgTypes> void TestMessage(const char *format, ArgTypes... Args) {
    char text[512];
    sprintf_s(text, format, Args...);
    QMessageBox msgBox;
    msgBox.setWindowTitle("Test message");
    msgBox.setText(text);
    msgBox.exec();
}

template <typename... ArgTypes> void TestMessage(wchar_t *format, ArgTypes... Args) {
    wchar_t text[512];
    swprintf_s(text, format, Args...);
    QMessageBox msgBox;
    msgBox.setWindowTitle("Test message");
    msgBox.setText(QString().fromWCharArray(text));
    msgBox.exec();
}

void __inline TestMessage(QString &text) {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Test message");
    msgBox.setText(text);
    msgBox.exec();
}