#include "snapshotapp.h"
#include "MapObject.h" // Включите, если MapObject вынесен в отдельный файл
#include "capturethread.h" // Включите заголовочный файл потока

#include <curl/curl.h> // Включите здесь для curl_global_init/cleanup

SnapshotApp::SnapshotApp(QWidget *parent) : QWidget(parent), captureThread(nullptr)
{
    setWindowTitle("Снимки карты по объектам");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // --- Панель добавления объекта ---
    QGroupBox *addObjectGroup = new QGroupBox("Добавить объект на карту");
    QVBoxLayout *objectGroupLayout = new QVBoxLayout();

    objectNameEdit = new QLineEdit("Объект1");
    objectGroupLayout->addWidget(new QLabel("Имя объекта (уникальное):"));
    objectGroupLayout->addWidget(objectNameEdit);

    latEdit = new QLineEdit("45.07");
    QDoubleValidator *latValidator = new QDoubleValidator(-90.0, 90.0, 8, this);
    latValidator->setLocale(QLocale::c());
    latEdit->setValidator(latValidator);
    objectGroupLayout->addWidget(new QLabel("Широта центра:"));
    objectGroupLayout->addWidget(latEdit);

    lonEdit = new QLineEdit("39.0");
    QDoubleValidator *lonValidator = new QDoubleValidator(-180.0, 180.0, 8, this);
    lonValidator->setLocale(QLocale::c());
    lonEdit->setValidator(lonValidator);
    objectGroupLayout->addWidget(new QLabel("Долгота центра:"));
    objectGroupLayout->addWidget(lonEdit);

    radiusEdit = new QLineEdit("5"); // 5 км по умолчанию
    radiusEdit->setValidator(new QIntValidator(1, 1000, this));
    objectGroupLayout->addWidget(new QLabel("Радиус съемки объекта (км):"));
    objectGroupLayout->addWidget(radiusEdit);

    // Поле для указания пути сохранения для ЭТОГО объекта
    objectSaveDirEdit = new QLineEdit("./screenshots_output/Объект1"); // Путь по умолчанию
    objectGroupLayout->addWidget(new QLabel("Путь для сохранения снимков объекта:"));
    QHBoxLayout *objectDirLayout = new QHBoxLayout();
    objectDirLayout->addWidget(objectSaveDirEdit);
    QPushButton *browseObjectDirButton = new QPushButton("Обзор...");
    connect(browseObjectDirButton, &QPushButton::clicked, this, &SnapshotApp::browseObjectDirectory);
    objectDirLayout->addSpacing(5);
    objectDirLayout->addWidget(browseObjectDirButton);
    objectGroupLayout->addLayout(objectDirLayout);

    addMapObjectButton = new QPushButton("Добавить объект");
    connect(addMapObjectButton, &QPushButton::clicked, this, &SnapshotApp::onAddMapObject);
    objectGroupLayout->addWidget(addMapObjectButton);

    addObjectGroup->setLayout(objectGroupLayout);
    mainLayout->addWidget(addObjectGroup);

    // --- Список объектов ---
    mapObjectsListWidget = new QListWidget();
    mainLayout->addWidget(new QLabel("Список объектов для съемки:"));
    mainLayout->addWidget(mapObjectsListWidget);

    removeMapObjectButton = new QPushButton("Удалить выбранный объект");
    connect(removeMapObjectButton, &QPushButton::clicked, this, &SnapshotApp::onRemoveMapObject);
    mainLayout->addWidget(removeMapObjectButton);

    // --- Общие настройки (без пути сохранения) ---
    QGroupBox *settingsGroup = new QGroupBox("Общие настройки съемки");
    QVBoxLayout *settingsLayout = new QVBoxLayout();

    startTimeEdit = new QTimeEdit(QTime::fromString("07:00", "hh:mm"));
    startTimeEdit->setDisplayFormat("hh:mm");
    settingsLayout->addWidget(new QLabel("Время начала (hh:mm):"));
    settingsLayout->addWidget(startTimeEdit);

    endTimeEdit = new QTimeEdit(QTime::fromString("23:00", "hh:mm"));
    endTimeEdit->setDisplayFormat("hh:mm");
    settingsLayout->addWidget(new QLabel("Время окончания (hh:mm):"));
    settingsLayout->addWidget(endTimeEdit);

    intervalEdit = new QLineEdit("60"); // 1 час
    intervalEdit->setValidator(new QIntValidator(1, 1440, this)); // от 1 мин до 24 часов
    settingsLayout->addWidget(new QLabel("Интервал съемки (м):"));
    settingsLayout->addWidget(intervalEdit);

    settingsGroup->setLayout(settingsLayout);
    mainLayout->addWidget(settingsGroup);

    // --- Кнопки управления ---
    QHBoxLayout *controlButtonsLayout = new QHBoxLayout();
    startButton = new QPushButton("Начать съемку");
    connect(startButton, &QPushButton::clicked, this, &SnapshotApp::startCapture);
    controlButtonsLayout->addWidget(startButton);

    stopButton = new QPushButton("Остановить съемку");
    connect(stopButton, &QPushButton::clicked, this, &SnapshotApp::stopCapture);
    controlButtonsLayout->addWidget(stopButton);

    mainLayout->addLayout(controlButtonsLayout);
    setLayout(mainLayout);

    // Обновление пути сохранения объекта при изменении имени
    connect(objectNameEdit, &QLineEdit::textChanged, this, &SnapshotApp::updateObjectSaveDir);

    curl_global_init(CURL_GLOBAL_DEFAULT);
}

SnapshotApp::~SnapshotApp()
{
    stopCapture(); // Остановить поток, если он работает
    if (captureThread)
    {
        captureThread->wait(5000); // Ждать завершения потока (с таймаутом)
        delete captureThread;
        captureThread = nullptr;
    }
    curl_global_cleanup();
}

void SnapshotApp::onAddMapObject()
{
    bool ok_lat, ok_lon, ok_radius;
    QString name_str = objectNameEdit->text().trimmed();
    double lat = latEdit->text().toDouble(&ok_lat);
    double lon = lonEdit->text().toDouble(&ok_lon);
    int radius_val = radiusEdit->text().toInt(&ok_radius);
    QString save_dir_str = objectSaveDirEdit->text().trimmed();

    if (name_str.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Имя объекта не может быть пустым.");
        return;
    }

    for (const auto &item : m_mapObjectList)
    {
        if (item.name == name_str.toStdString())
        {
            QMessageBox::warning(this, "Ошибка", "Объект с таким именем уже существует.");
            return;
        }
    }

    if (!ok_lat || !ok_lon || !ok_radius || radius_val <= 0)
    {
        QMessageBox::warning(this, "Ошибка", "Неверные параметры для объекта (широта, долгота или радиус).");
        return;
    }

    if (save_dir_str.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Путь для сохранения объекта не может быть пустым.");
        return;
    }

    MapObject newObj(lat, lon, radius_val, name_str.toStdString(), save_dir_str.toStdString());
    m_mapObjectList.push_back(newObj);
    mapObjectsListWidget->addItem(newObj.getDisplayText());

    int next_obj_num = 1;
    if (name_str.startsWith("Объект") && name_str.mid(6).toInt() > 0)
    {
        next_obj_num = name_str.mid(6).toInt() + 1;
    }
    else
    {
        next_obj_num = m_mapObjectList.size() + 1;
    }
    objectNameEdit->setText("Объект" + QString::number(next_obj_num));
    updateObjectSaveDir(objectNameEdit->text());

    latEdit->setText("0.0");
    lonEdit->setText("0.0");
    radiusEdit->setText("5");

    std::cout << "Добавлен объект: " << newObj.name << " в каталог " << newObj.save_directory << std::endl;
}

void SnapshotApp::onRemoveMapObject()
{
    int currentRow = mapObjectsListWidget->currentRow();
    if (currentRow >= 0 && currentRow < static_cast<int>(m_mapObjectList.size()))
    {
        std::cout << "Удаление объекта: " << m_mapObjectList[currentRow].name << std::endl;
        m_mapObjectList.erase(m_mapObjectList.begin() + currentRow);
        delete mapObjectsListWidget->takeItem(currentRow);
        std::cout << "Удален объект с индексом " << currentRow << std::endl;
    }
    else
    {
        QMessageBox::information(this, "Информация", "Выберите объект для удаления.");
    }
}

void SnapshotApp::startCapture()
{
    if (captureThread && captureThread->isRunning())
    {
        QMessageBox::warning(this, "Предупреждение", "Съемка уже запущена.");
        return;
    }

    if (m_mapObjectList.empty())
    {
        QMessageBox::warning(this, "Предупреждение", "Список объектов для съемки пуст. Добавьте хотя бы один объект.");
        return;
    }

    bool ok_interval;
    std::string current_start_time = startTimeEdit->time().toString("hh:mm").toStdString();
    std::string current_end_time = endTimeEdit->time().toString("hh:mm").toStdString();
    int current_capture_interval = intervalEdit->text().toInt(&ok_interval) * 60;

    if (!ok_interval || current_capture_interval < 60)
    {
        QMessageBox::critical(this, "Ошибка", "Неверное значение интервала съемки (минимум 60 секунд).");
        return;
    }

    for (const auto &obj : m_mapObjectList)
    {
        if (obj.save_directory.empty())
        {
            QMessageBox::critical(this, "Ошибка", QString("Путь сохранения для объекта '%1' пуст.").arg(QString::fromStdString(obj.name)));
            return;
        }
    }

    if (captureThread)
    {
        captureThread->wait(1000);
        delete captureThread;
        captureThread = nullptr;
    }

    captureThread = new CaptureThread(m_mapObjectList,
                                      current_capture_interval,
                                      current_start_time,
                                      current_end_time,
                                      this);

    connect(captureThread, &QThread::finished, captureThread, &QObject::deleteLater);
    connect(captureThread, &QThread::finished, this, [this]()
            {
                this->startButton->setEnabled(true);
                this->stopButton->setEnabled(false);
                this->addMapObjectButton->setEnabled(true);
                this->removeMapObjectButton->setEnabled(true);
                QMessageBox::information(this, "Статус", "Процесс съемки завершен."); });

    captureThread->start();

    startButton->setEnabled(false);
    stopButton->setEnabled(true);
    addMapObjectButton->setEnabled(false);
    removeMapObjectButton->setEnabled(false);

    QMessageBox::information(this, "Статус", "Съемка начата.");
}

void SnapshotApp::stopCapture()
{
    if (captureThread && captureThread->isRunning())
    {
        std::cout << "Нажата кнопка 'Остановить'. Сигнализируем потоку." << std::endl;
        captureThread->stop();
        stopButton->setEnabled(false);
        QMessageBox::information(this, "Статус", "Запрос на остановку съемки отправлен. Поток завершит текущую операцию и остановится.");
    }
    else
    {
        if (!captureThread)
        {
            startButton->setEnabled(true);
            stopButton->setEnabled(false);
            addMapObjectButton->setEnabled(true);
            removeMapObjectButton->setEnabled(true);
        }
        QMessageBox::information(this, "Информация", "Съемка не запущена.");
    }
}

void SnapshotApp::browseObjectDirectory()
{
    QString currentObjectDir = objectSaveDirEdit->text().trimmed();
    if (currentObjectDir.isEmpty())
    {
        currentObjectDir = "./screenshots_output/" + objectNameEdit->text().trimmed();
    }

    QString directory = QFileDialog::getExistingDirectory(this, "Выбор каталога для объекта", currentObjectDir);

    if (!directory.isEmpty())
    {
        objectSaveDirEdit->setText(directory);
    }
}

void SnapshotApp::updateObjectSaveDir(const QString &name)
{
    QString base_dir = "./screenshots_output/";
    QString safe_name = name.trimmed();

    if (safe_name.isEmpty())
        safe_name = "Объект";

    safe_name.replace(" ", "_");
    safe_name.replace("/", "_");

    objectSaveDirEdit->setText(base_dir + safe_name);
}
