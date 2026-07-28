#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "aimanager.h"
#include "chathistory.h"

// Для интерфейса
class QListWidget;
class QLineEdit;
class QPushButton;
class QListWidgetItem;
class QLabel;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;  // для обновления ширины облачков

private slots:
    void slotSendButtonClicked();                   // Отправка вопроса нейросетке по клику

    // Слоты от AIManager
    void slotChunkReceived(const QString &chunk);
    void slotResponseFinished(const QString &fullText);
    void slotErrorOccurred(const QString &errorString);

private:
    // UI
    QListWidget *chat_display_;
    QLineEdit *input_question_;
    QPushButton *button_send_question_;

    // Менеджеры
    AIManager *ai_manager_;
    ChatHistory *history_;

    // Для отображения текущего сообщения ассистента
    QListWidgetItem *current_item_ = nullptr;
    QWidget *current_container_ = nullptr;
    QLabel *current_label_ = nullptr;
    QString accumulated_answer_;

    void CreateUI();
    void AddMessage(const QString &text, bool isUser);
    void CreateOrUpdateAssistantMessage(const QString &text);
};

#endif // MAINWINDOW_H
