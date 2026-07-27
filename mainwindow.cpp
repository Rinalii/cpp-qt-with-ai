#include "mainwindow.h"

// Для интерфейса
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
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
    chat_display_ = new QListWidget(this);
    chat_display_->setUniformItemSizes(false);
    chat_display_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

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

void MainWindow::AddMessage(const QString &text, bool isUser) {
    QWidget *container = new QWidget();
    QHBoxLayout *container_layout = new QHBoxLayout(container);
    container_layout->setContentsMargins(10, 5, 10, 5);

    QFrame *bubble = new QFrame();
    bubble->setFrameShape(QFrame::NoFrame);
    QString bgColor = isUser ? "#DCF8C6" : "#FFFFFF";
    QString borderColor = isUser ? "#8BC34A" : "#E0E0E0";

    bubble->setStyleSheet(QString(
                              "background-color: %1; "
                              "border-radius: 15px; "
                              "border: none; "
                              "padding: 0px;"
                              ).arg(bgColor));

    QLabel *label = new QLabel();
    label->setTextFormat(Qt::MarkdownText);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    label->setWordWrap(true);
    label->setText(text);
    label->setStyleSheet("background: transparent; color: black; margin: 0px; padding: 8px;");
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    // Ограничиваем максимальную ширину - 70% от ширины списка
    int maxWidth = chat_display_->width() * 0.7;
    if (maxWidth < 50) maxWidth = 50;                   // защита от нуля
    label->setMaximumWidth(maxWidth);

    // Layout для bubble
    QVBoxLayout *bubble_layout = new QVBoxLayout(bubble);
    bubble_layout->setContentsMargins(0, 0, 0, 0);
    bubble_layout->addWidget(label);

    // Выравнивание
    if (isUser) {
        container_layout->addStretch();
        container_layout->addWidget(bubble);
    } else {
        container_layout->addWidget(bubble);
        container_layout->addStretch();
    }

    // Добавляем в список
    QListWidgetItem *item = new QListWidgetItem(chat_display_);
    chat_display_->setItemWidget(item, container);

    // Принудительно обновляем геометрию, чтобы label получил актуальный размер
    container->adjustSize();
    item->setSizeHint(container->sizeHint());
    chat_display_->scrollToBottom();

    if (!isUser){
        // Сохраняем указатели для последующего обновления
        current_item_ = item;
        current_container_ = container;
        current_label_ = label;
    }
}

void MainWindow::CreateOrUpdateAssistantMessage(const QString &text) {
    if (current_item_ == nullptr) {
        // Создаём новый элемент для сообщения ассистента
        AddMessage(text, false);
    } else {
        // Обновляем текст существующего сообщения
        current_label_->setText(text);
        current_container_->adjustSize();
        current_item_->setSizeHint(current_container_->sizeHint());
        chat_display_->scrollToBottom();
    }
}


void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    int max_width = chat_display_->width() * 0.7;
    if (max_width < 50) max_width = 50;

    for (int i = 0; i < chat_display_->count(); ++i) {
        QListWidgetItem *item = chat_display_->item(i);
        QWidget *widget = chat_display_->itemWidget(item);
        if (!widget) continue;
        // Ищем QLabel внутри container
        QLabel *label = widget->findChild<QLabel*>();
        if (label) {
            label->setMaximumWidth(max_width);
        }
        // Обновляем размер элемента
        widget->adjustSize();
        item->setSizeHint(widget->sizeHint());
    }

    // Также обновляем текущее сообщение, если оно есть
    if (current_label_) {
        current_label_->setMaximumWidth(max_width);
        current_container_->adjustSize();
        current_item_->setSizeHint(current_container_->sizeHint());
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , manager_(new QNetworkAccessManager(this))
    , history_(QJsonArray()) {

    CreateUI();

    // Назначаем сигналы и слоты
    // По клику на кнопку или enter в QLineEdit - отправка запроса
    connect(button_send_question_, &QPushButton::clicked, this, &MainWindow::slotSendButtonClicked);
    connect(input_question_, &QLineEdit::returnPressed, this, &MainWindow::slotSendButtonClicked);

    // Устанавливаем размер окна
    resize(600, 400);

    // Добавляем системное сообщение один раз в начале истории
    QJsonObject system_msg;
    system_msg["role"] = "system";
    system_msg["content"] =
        "You're an AI assistant. Be nice, keep up the conversation, answer the user's questions.";

    history_.append(system_msg);
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
    AddMessage(user_input, true);
    button_send_question_->setEnabled(false); // Блокируем кнопку на время ожидания

    // Добавляем новый вопрос пользователя в историю
    QJsonObject user_msg;
    user_msg["role"] = "user";
    user_msg["content"] = user_input;
    history_.append(user_msg);

    // Формируем запрос к /api/chat, чтобы передавать историю диалога
    QNetworkRequest request;
    request.setUrl(QUrl("http://localhost:11434/api/chat"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Собираем JSON запроса
    QJsonObject json;
    json["model"] = "gemma-4-12b-local";        // Запускаемая модель
    json["messages"] = history_;                // вся история
    json["stream"] = true;                      // Включаем потоковый режим

    QByteArray post_data = QJsonDocument(json).toJson();
    QNetworkReply *reply = manager_->post(request, post_data);

    // Сохраняем указатель на текущий ответ
    current_reply_ = reply;
    accumulated_answer_.clear();
    read_buffer_.clear();

    // Сбрасываем указатели на элементы чата
    current_item_ = nullptr;
    current_container_ = nullptr;
    current_label_ = nullptr;

    // Подключаем сигналы для потокового чтения
    connect(reply, &QNetworkReply::readyRead, this, &MainWindow::slotReadyRead);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::slotStreamFinished);
    connect(reply, &QNetworkReply::errorOccurred, this, &MainWindow::slotStreamError);
}

// Слот для чтения очередного куска данных
void MainWindow::slotReadyRead() {
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
            // Обновляем или создаём сообщение ассистента
            CreateOrUpdateAssistantMessage(accumulated_answer_);
        }
    }
}

// Слот для получения ответа от AI
void MainWindow::slotStreamFinished() {
    if (!current_reply_) return;

    if (current_reply_->error() == QNetworkReply::NoError) {
        // Если ответ не был добавлен, создаём сейчас
        if (!accumulated_answer_.isEmpty() && current_item_ == nullptr) {
            CreateOrUpdateAssistantMessage(accumulated_answer_);
        }

        // Добавляем финальный ответ в историю
        if (!accumulated_answer_.isEmpty()) {
            QJsonObject assistant_msg;
            assistant_msg["role"] = "assistant";
            assistant_msg["content"] = accumulated_answer_;
            history_.append(assistant_msg);

            // Ограничиваем историю последними 20 сообщениями
            while (history_.size() > 20) {
                history_.removeAt(1);
            }
        } else {
            // Пустой ответ
            AddMessage("[пустой ответ]", false);
        }
    } else {
        AddMessage("Соединение с Ollama не удалось: " + current_reply_->errorString() +
                                 "\nУбедитесь, что Ollama запущен и модель загружена.", false);
    }

    // Очищаем состояние
    current_reply_->deleteLater();
    current_reply_ = nullptr;
    current_item_ = nullptr;
    current_container_ = nullptr;
    current_label_ = nullptr;
    accumulated_answer_.clear();
    read_buffer_.clear();

    button_send_question_->setEnabled(true); // Разблокируем кнопку
}

// Слот обработки ошибок сети
void MainWindow::slotStreamError(QNetworkReply::NetworkError code) {
    Q_UNUSED(code);
    if (current_reply_) {
        AddMessage("Ошибка сети: " + current_reply_->errorString() +
                       "\nУбедитесь, что Ollama запущен и модель загружена.", false);

        current_reply_->deleteLater();
        current_reply_ = nullptr;
        current_item_ = nullptr;
        current_container_ = nullptr;
        current_label_ = nullptr;
        accumulated_answer_.clear();
        read_buffer_.clear();
        button_send_question_->setEnabled(true);
    }
}
