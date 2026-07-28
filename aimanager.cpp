#include "aimanager.h"

#include <QJsonDocument>
#include <QJsonObject>

AIManager::AIManager(const QString &base_url, const QString &model, QObject *parent) :
    QObject(parent)
    , manager_(new QNetworkAccessManager(this))
    , base_url_(base_url)
    , model_(model) {

}

void AIManager::SendRequest(const QJsonArray &messages) {
    // Если уже есть незавершённый запрос – прерываем его
    if (current_reply_) {
        current_reply_->abort();
        current_reply_->deleteLater();
        current_reply_ = nullptr;
    }

    accumulated_answer_.clear();
    read_buffer_.clear();

    // Формируем запрос к /api/chat, чтобы передавать историю диалога
    QNetworkRequest request;
    request.setUrl(QUrl(base_url_ + "/api/chat"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Собираем JSON запроса
    QJsonObject json;
    json["model"] = model_;         // Запускаемая модель
    json["messages"] = messages;    // вся история
    json["stream"] = true;          // Включаем потоковый режим

    QByteArray postData = QJsonDocument(json).toJson();
    // Сохраняем указатель на текущий ответ
    current_reply_ = manager_->post(request, postData);

    // Подключаем сигналы для потокового чтения
    connect(current_reply_, &QNetworkReply::readyRead, this, &AIManager::slotReadyRead);
    connect(current_reply_, &QNetworkReply::finished, this, &AIManager::slotStreamFinished);
    connect(current_reply_, &QNetworkReply::errorOccurred, this, &AIManager::slotStreamError);
}

// Слот для чтения очередного куска данных
void AIManager::slotReadyRead() {
    if (!current_reply_) return;

    // Читаем все доступные данные и добавляем в буфер
    read_buffer_.append(current_reply_->readAll());

    // Разбираем буфер построчно (каждая строка — отдельный JSON)
    int pos;
    while ((pos = read_buffer_.indexOf('\n')) != -1) {
        QByteArray line = read_buffer_.left(pos);
        read_buffer_.remove(0, pos + 1);

        if (line.trimmed().isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || doc.isNull()) {
            // Если JSON невалидный, пропускаем
            continue;
        }

        QJsonObject obj = doc.object();
        QJsonObject message = obj["message"].toObject();
        QString chunk = message["content"].toString();
        if (!chunk.isEmpty()) {
            accumulated_answer_.append(chunk);
            // Подаем сигнал с полученной частью сообщения
            emit signalChunkReceived(chunk);
        }
    }
}

// Слот для получения ответа от AI
void AIManager::slotStreamFinished() {
    if (!current_reply_) return;

    if (current_reply_->error() == QNetworkReply::NoError) {
        emit signalResponseFinished(accumulated_answer_);
    } else {
        emit signalErrorOccurred(current_reply_->errorString());
    }

    // Очищаем состояние
    current_reply_->deleteLater();
    current_reply_ = nullptr;
    accumulated_answer_.clear();
    read_buffer_.clear();
}

// Слот обработки ошибок сети
void AIManager::slotStreamError(QNetworkReply::NetworkError code) {
    Q_UNUSED(code);
    if (current_reply_) {
        emit signalErrorOccurred(current_reply_->errorString());

        current_reply_->deleteLater();
        current_reply_ = nullptr;
        accumulated_answer_.clear();
        read_buffer_.clear();
    }
}
