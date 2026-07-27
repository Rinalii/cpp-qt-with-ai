#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonArray>
#include <QNetworkReply>

// Для интерфейса
class QListWidget;
class QLineEdit;
class QPushButton;
class QVBoxLayout;
class QListWidgetItem;
class QLabel;

// Для работы с нейросетями
class QNetworkAccessManager;
class QNetworkReply;

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

    // Обработка ответа от AI
    void slotReadyRead();
    void slotStreamFinished();
    void slotStreamError(QNetworkReply::NetworkError code);

private:
    // Виджеты интерфейса
    QListWidget *chat_display_;
    QLineEdit *input_question_;
    QPushButton *button_send_question_;

    // Сетевой менеджер
    QNetworkAccessManager *manager_;

    // История сообщений
    QJsonArray history_;

    // Для потокового получения ответа
    QNetworkReply *current_reply_ = nullptr;
    QString accumulated_answer_;
    QListWidgetItem *current_item_ = nullptr;   // элемент списка для обновляемого сообщения
    QWidget *current_container_ = nullptr;
    QLabel *current_label_ = nullptr;
    QByteArray read_buffer_;                    // буфер для неполных строк


    void CreateUI();
    void AddMessage(const QString &text, bool isUser);
    void CreateOrUpdateAssistantMessage(const QString &text);
};

#endif // MAINWINDOW_H
