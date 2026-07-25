#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonArray>

// Для интерфейса
class QTextEdit;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

// Для работы с нейросетями
class QNetworkAccessManager;
class QNetworkReply;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void slotSendButtonClicked();                   // Отправка вопроса нейросетке по клику
    void slotReplyFinished(QNetworkReply *reply);   // Обработка ответа от AI

private:
    // Виджеты интерфейса
    QTextEdit *chat_display_;
    QLineEdit *input_question_;
    QPushButton *button_send_question_;

    // Сетевой менеджер
    QNetworkAccessManager *manager_;

    // История сообщений
    QJsonArray history_;

    void CreateUI();
};

#endif // MAINWINDOW_H
