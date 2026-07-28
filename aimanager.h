#ifndef AIMANAGER_H
#define AIMANAGER_H

#include <QMainWindow>
#include <QJsonArray>
#include <QNetworkReply>

// Для работы с нейросетями
class QNetworkAccessManager;
class QNetworkReply;

class AIManager : public QObject {
    Q_OBJECT

public:
    explicit AIManager(const QString &base_url = "http://localhost:11434",
                       const QString &model = "gemma-4-12b-local",
                       QObject *parent = nullptr);
    ~AIManager() = default;

    void SendRequest(const QJsonArray &messages); // отправляет запрос с переданными сообщениями

signals:
    void signalChunkReceived(const QString &chunk);
    void signalResponseFinished(const QString &fullText);
    void signalErrorOccurred(const QString &error);

private slots:
    // Обработка ответа от AI
    void slotReadyRead();
    void slotStreamFinished();
    void slotStreamError(QNetworkReply::NetworkError code);

private:
    // Сетевой менеджер
    QNetworkAccessManager *manager_;
    QString base_url_;
    QString model_;

    // Для потокового получения ответа
    QNetworkReply *current_reply_ = nullptr;
    QString accumulated_answer_;
    QByteArray read_buffer_;                    // буфер для неполных строк
};

#endif // AIMANAGER_H
