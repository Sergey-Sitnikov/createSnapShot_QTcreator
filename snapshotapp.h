#ifndef SNAPSHOTAPP_H
#define SNAPSHOTAPP_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTimeEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QListWidget>
#include <QThread>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QGroupBox>

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

#include "MapObject.h" 
#include "capturethread.h" //заголовочный файл потока

// Forward declaration для CURL, если curl_global_init/cleanup используются здесь
// struct CURL; // Не нужно, если используем только функции из curl.h, которые определены с extern "C"

class CaptureThread; // Forward declaration для потока

class SnapshotApp : public QWidget
{
    Q_OBJECT // Макрос Q_OBJECT для поддержки сигналов и слотов

public:
    SnapshotApp(QWidget *parent = nullptr);
    ~SnapshotApp();

private slots:
    void onAddMapObject();
    void onRemoveMapObject();
    void startCapture();
    void stopCapture();
    void browseObjectDirectory();
    void updateObjectSaveDir(const QString &name);

private:
    // Элементы GUI для добавления объекта
    QLineEdit *objectNameEdit;
    QLineEdit *latEdit;
    QLineEdit *lonEdit;
    QLineEdit *radiusEdit;
    QLineEdit *objectSaveDirEdit; // Поле для пути сохранения объекта
    QPushButton *addMapObjectButton;

    // Список объектов
    QListWidget *mapObjectsListWidget;
    QPushButton *removeMapObjectButton;

    std::vector<MapObject> m_mapObjectList; // Хранилище объектов

    // Общие настройки
    QTimeEdit *startTimeEdit;
    QTimeEdit *endTimeEdit;
    QLineEdit *intervalEdit;

    // Кнопки управления
    QPushButton *startButton;
    QPushButton *stopButton;

    CaptureThread *captureThread; // Указатель на поток захвата
};

#endif // SNAPSHOTAPP_H
