#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include <QThread>
#include <string>
#include <vector>
#include <utility> // для std::pair
#include <ctime>   // для std::tm
#include <atomic> // для std::atomic_bool
#include <locale> // для std::locale
#include <sstream> // Для std::ostringstream
#include <cmath>   // для std::cos, M_PI
#include <limits> // для std::numeric_limits
#include <filesystem> // для std::filesystem
#include <iostream>   // для std::cerr, std::cout
#include <algorithm>  // для std::replace_if
#include <vector>     // для std::vector
#include <cerrno>     // для errno
#include <cstdio>     // для FILE, fopen, fclose
#include <thread>   // Для std::this_thread::sleep_for
#include <fstream>  // Для std::ifstream

#include <curl/curl.h> // для CURL
#include "MapObject.h"

// Forward declaration для MapObject, если MapObject не выносится в отдельный файл
// Если MapObject вынесен, включите его заголовочный файл
class MapObject;

// Вспомогательная функция для объединения изображений
bool combineAndCleanupScreenshots(const std::string &temp_dir_path,
                                  const std::string &output_dir_path,
                                  std::tm *current_time,
                                  const std::string &object_name_identifier);

class CaptureThread : public QThread
{
    Q_OBJECT // Макрос Q_OBJECT для поддержки сигналов и слотов

public:
    CaptureThread(std::vector<MapObject> objects,
                  int interval,
                  std::string st_time,
                  std::string en_time,
                  QObject *parent = nullptr);

    void stop(); // Метод для запроса остановки потока

protected:
    void run() override; // Основная функция потока

private:
    std::vector<MapObject> m_mapObjects;
    int m_capture_interval_sec;
    std::string m_start_time_str;
    std::string m_end_time_str;
    std::atomic_bool running; // Атомарная переменная для безопасной остановки

    void sleepAndCheckRunning(int seconds);
    bool isWithinCaptureTimeWindow(const std::tm *current_time_tm);
    void generateCoordinatesForObject(const MapObject &obj, std::vector<std::pair<double, double>> &out_coords);
    void createSnapshot(std::pair<double, double> bottom_left_coord, const std::string &directory, const std::string &format, int index, std::tm *current_time_tm);
};

#endif // CAPTURETHREAD_H
