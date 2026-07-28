#ifndef CHATHISTORY_H
#define CHATHISTORY_H

#include <QObject>

#include <QObject>
#include <QJsonArray>
#include <QString>

class ChatHistory : public QObject
{
    Q_OBJECT
public:
    explicit ChatHistory(const QString &system_prompt = "", QObject *parent = nullptr);

    // Добавить сообщение (role: "user" или "assistant")
    void AddMessage(const QString &role, const QString &content);
    void SetSystemPrompt(const QString &prompt);

    QJsonArray GetMessagesForRequest(int maxCount) const;

    int getSize() const;
    void Clear();

signals:
    void signalHistoryChanged();

private:
    QList<QJsonObject> messages_;   // user / assistant
    QString system_prompt_;
};

#endif // CHATHISTORY_H
