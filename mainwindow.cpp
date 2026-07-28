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

    QLabel *label = new QLabel();
    label->setTextFormat(Qt::MarkdownText);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    label->setWordWrap(true);
    label->setText(text);

    QString bgColor = isUser ? "#DCF8C6" : "#FFFFFF";
    label->setStyleSheet(QString(
                             "background-color: %1; "
                             "border-radius: 15px; "
                             "padding: 8px; "
                             "color: black;"
                             ).arg(bgColor));

    // Ограничиваем максимальную ширину - 70% от ширины списка
    int maxWidth = chat_display_->width() * 0.7;
    if (maxWidth < 50) maxWidth = 50;                   // защита от нуля
    label->setMaximumWidth(maxWidth);
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    // Выравнивание
    if (isUser) {
        container_layout->addStretch();
        container_layout->addWidget(label);
    } else {
        container_layout->addWidget(label);
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
    , ai_manager_(new AIManager("http://localhost:11434", "gemma-4-12b-local", this)) {

    history_ = new ChatHistory("You're an AI assistant. Be nice, keep up the conversation, answer the user's questions.", this);

    CreateUI();

    // Назначаем сигналы и слоты
    // По клику на кнопку или enter в QLineEdit - отправка запроса
    connect(button_send_question_, &QPushButton::clicked, this, &MainWindow::slotSendButtonClicked);
    connect(input_question_, &QLineEdit::returnPressed, this, &MainWindow::slotSendButtonClicked);

    connect(ai_manager_, &AIManager::signalChunkReceived, this, &MainWindow::slotChunkReceived);
    connect(ai_manager_, &AIManager::signalResponseFinished, this, &MainWindow::slotResponseFinished);
    connect(ai_manager_, &AIManager::signalErrorOccurred, this, &MainWindow::slotErrorOccurred);

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

    history_->AddMessage("user", user_input);
    // Показываем вопрос пользователя в чате
    AddMessage(user_input, true);
    button_send_question_->setEnabled(false); // Блокируем кнопку на время ожидания

    // Сбрасываем указатели на элементы чата
    current_item_ = nullptr;
    current_container_ = nullptr;
    current_label_ = nullptr;
    accumulated_answer_.clear();

    // Берём последние 20 сообщений (системное + 19 последних)
    QJsonArray recent = history_->GetMessagesForRequest(20);
    ai_manager_->SendRequest(recent);
}

// Слот для чтения очередного куска данных
void MainWindow::slotChunkReceived(const QString &chunk)
{
    accumulated_answer_.append(chunk);
    // Обновляем или создаём сообщение ассистента
    CreateOrUpdateAssistantMessage(accumulated_answer_);
}

// Слот для получения финального ответа от AI
void MainWindow::slotResponseFinished(const QString &fullText)
{
    QString finalAnswer = fullText.isEmpty() ? accumulated_answer_ : fullText;

    if (!finalAnswer.isEmpty()) {
        // Добавляем финальный ответ в историю
        history_->AddMessage("assistant", finalAnswer);
        if (current_item_ == nullptr) {
            CreateOrUpdateAssistantMessage(finalAnswer);
        } else {
            current_label_->setText(finalAnswer);
            current_container_->adjustSize();
            current_item_->setSizeHint(current_container_->sizeHint());
            chat_display_->scrollToBottom();
        }
    } else {
        // Пустой ответ
        AddMessage("[пустой ответ]", false);
    }

    current_item_ = nullptr;
    current_container_ = nullptr;
    current_label_ = nullptr;
    accumulated_answer_.clear();

    button_send_question_->setEnabled(true); // Разблокируем кнопку
}

void MainWindow::slotErrorOccurred(const QString &error_string)
{
    AddMessage("Ошибка сети: " + error_string +
                   "\nУбедитесь, что Ollama запущен и модель загружена.", false);

    current_item_ = nullptr;
    current_container_ = nullptr;
    current_label_ = nullptr;
    accumulated_answer_.clear();

    button_send_question_->setEnabled(true);
}
