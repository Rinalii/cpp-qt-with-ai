#include "mainwindow.h"

// Для интерфейса
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

// Для работы с нейросетями
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

// Для JSON
#include <QJsonObject>
#include <QJsonDocument>

#include <QUrl>
#include <QString>


void MainWindow::CreateUI() {
    // Создаём элементы интерфейса
    chat_display_ = new QTextEdit(this);
    chat_display_->setReadOnly(true);                                   // Только для чтения
    chat_display_->setPlaceholderText("Здесь будет диалог с AI...");

    input_question_ = new QLineEdit(this);
    input_question_->setPlaceholderText("Введите вопрос...");

    button_send_question_ = new QPushButton("Отправить", this);

    // Горизонтально поле ввода и кнопка отправки
    QHBoxLayout *bottom_layout = new QHBoxLayout();
    bottom_layout->addWidget(input_question_);
    bottom_layout->addWidget(button_send_question_);

    // Создаём центральный виджет
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // Назначаем верт. Layout центральному виджету
    QVBoxLayout *main_layout = new QVBoxLayout(central);
    main_layout->addWidget(chat_display_);
    main_layout->addLayout(bottom_layout);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , manager_(new QNetworkAccessManager(this)) {

    CreateUI();

    // Назначаем сигналы и слоты
    // По клику на кнопку или enter в QLineEdit - отправка запроса
    connect(button_send_question_, &QPushButton::clicked, this, &MainWindow::slotSendButtonClicked);
    connect(input_question_, &QLineEdit::returnPressed, this, &MainWindow::slotSendButtonClicked);

    // По готовности ответа от нейросети - обработка ответа
    connect(manager_, &QNetworkAccessManager::finished, this, &MainWindow::slotReplyFinished);

    // Устанавливаем размер окна
    resize(600, 400);
}

MainWindow::~MainWindow() {}

// Слот для кнопки "Отправить"
void MainWindow::slotSendButtonClicked() {
    // Удаляем пробелы слева и справа, чтобы отсечь пустой ввод
    QString user_input = input_question_->text().trimmed();
    if (user_input.isEmpty()) {
        return;
    }
    input_question_->clear();

    // Показываем вопрос пользователя в чате
    chat_display_->append("Вы: " + user_input);
    button_send_question_->setEnabled(false); // Блокируем кнопку на время ожидания

    // Формируем запрос к локальному серверу Ollama
    QNetworkRequest request;
    request.setUrl(QUrl("http://localhost:11434/api/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Собираем JSON запроса
    QJsonObject json;
    json["model"] = "qwen3:8b";     // Запускаемая модель
    json["prompt"] = user_input;
    json["stream"] = false;         // Отключаем потоковый режим для простоты

    QByteArray post_data = QJsonDocument(json).toJson();
    manager_->post(request, post_data);     // Отправляем запрос нейросетке
}

// Слот для получения ответа от AI
void MainWindow::slotReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            QJsonObject obj = doc.object();
            QString answer = obj["response"].toString();
            if (!answer.isEmpty()) {
                chat_display_->append("AI: " + answer);
            } else {
                chat_display_->append("AI: [пустой ответ]");
            }
        } else {
            chat_display_->append("Ошибка: не удалось разобрать JSON");
        }
    } else {
        chat_display_->append("Ошибка соединения: " + reply->errorString());
        chat_display_->append("Убедитесь, что Ollama запущен и модель загружена.");
    }

    button_send_question_->setEnabled(true); // Разблокируем кнопку
    reply->deleteLater();
}
