#include "capturethread.h"
#include "snapshotapp.h" // Если SnapshotApp содержит MapObject или другие необходимые определения
#include "MapObject.h"   // Включите, если MapObject вынесен в отдельный файл

// Вспомогательная функция для объединения изображений (реализация)
#include <QDir>
#include <QFileInfoList>
#include <QStringList>
#include <QImage>
#include <QPainter>

bool combineAndCleanupScreenshots(const std::string &temp_dir_path,
                                  const std::string &output_dir_path, // Это базовый путь для объекта
                                  std::tm *current_time,
                                  const std::string &object_name_identifier)
{
    QDir tempDir(QString::fromStdString(temp_dir_path));
    if (!tempDir.exists())
    {
        std::cerr << "Временный каталог для объединения не существует: " << temp_dir_path << std::endl;
        std::error_code ec;
        std::filesystem::remove_all(temp_dir_path, ec); // Попытка очистки
        if (ec)
        {
            std::cerr << "Ошибка при очистке несуществующего временного каталога (filesystem): " << ec.message() << std::endl;
        }
        return false;
    }

    QStringList filters;
    filters << "*.png";
    QFileInfoList fileList = tempDir.entryInfoList(filters, QDir::Files, QDir::Name);

    if (fileList.isEmpty())
    {
        std::cerr << "Временные скриншоты в каталоге " << temp_dir_path << " не найдены. Объединение пропущено для объекта " << object_name_identifier << "." << std::endl;
        std::error_code ec;
        std::filesystem::remove_all(temp_dir_path, ec);
        if (ec)
        {
            std::cerr << "Ошибка при очистке пустого временного каталога (filesystem): " << ec.message() << std::endl;
        }
        return false;
    }

    std::sort(fileList.begin(), fileList.end(), [](const QFileInfo &a, const QFileInfo &b)
              {
                  QString name_a = a.baseName();
                  QString name_b = b.baseName();
                  int last_underscore_a = name_a.lastIndexOf('_');
                  int last_underscore_b = name_b.lastIndexOf('_');
                  int index_a = (last_underscore_a != -1) ? name_a.mid(last_underscore_a + 1).toInt() : -1;
                  int index_b = (last_underscore_b != -1) ? name_b.mid(last_underscore_b + 1).toInt() : -1;
                  return index_a < index_b; });

    int total_images = fileList.size();
    int N_grid_dim = static_cast<int>(std::sqrt(static_cast<double>(total_images)));

    if (N_grid_dim * N_grid_dim != total_images)
    {
        std::cerr << "Ошибка для объекта " << object_name_identifier << ": Общее количество скриншотов (" << total_images
                  << ") не является полным квадратом. Невозможно сформировать сетку " << N_grid_dim << "x" << N_grid_dim
                  << ". Временные файлы сохранены в " << temp_dir_path << std::endl;
        return false; // НЕ очищать временные файлы в этом случае
    }

    QImage firstImage(fileList.first().absoluteFilePath());
    if (firstImage.isNull())
    {
        std::cerr << "Ошибка загрузки первого изображения для объекта " << object_name_identifier << ": "
                  << fileList.first().absoluteFilePath().toStdString() << std::endl;
        std::error_code ec;
        std::filesystem::remove_all(temp_dir_path, ec);
        if (ec)
        {
            std::cerr << "Ошибка при очистке filesystem после сбоя загрузки первого изображения: " << ec.message() << std::endl;
        }
        return false;
    }

    int img_width = firstImage.width();
    int img_height = firstImage.height();
    int composite_width = N_grid_dim * img_width;
    int composite_height = N_grid_dim * img_height;

    QImage compositeImage(composite_width, composite_height, QImage::Format_RGB32);
    compositeImage.fill(Qt::white);
    QPainter painter(&compositeImage);

    for (int i = 0; i < total_images; ++i)
    {
        QImage img(fileList.at(i).absoluteFilePath());
        if (img.isNull())
        {
            std::cerr << "Ошибка загрузки изображения: " << fileList.at(i).absoluteFilePath().toStdString() << ". Пропуск." << std::endl;
            continue;
        }
        int row = i / N_grid_dim;
        int col = i % N_grid_dim;
        int composite_row = (N_grid_dim - 1) - row;
        painter.drawImage(col * img_width, composite_row * img_height, img);
    }
    painter.end();

    char time_str_buffer[80];
    std::strftime(time_str_buffer, sizeof(time_str_buffer), "%Y-%m-%d_%H-%M-%S", current_time);

    // Путь сохранения теперь берется из объекта, переданного потоку
    // Уникальное имя файла включает имя объекта
    std::string safe_object_name = object_name_identifier;
    std::replace_if(safe_object_name.begin(), safe_object_name.end(), [](char c)
                    { return !std::isalnum(c) && c != '_' && c != '-'; }, '_');

    std::string output_file_name = output_dir_path + "/" + safe_object_name + "_" + time_str_buffer + ".bmp";

    // Убедиться, что выходной каталог объекта существует перед сохранением
    std::error_code ec_dir;
    std::filesystem::create_directories(output_dir_path, ec_dir);
    if (ec_dir)
    {
        std::cerr << "Ошибка создания выходного каталога для объекта " << object_name_identifier << ": " << output_dir_path << " - " << ec_dir.message() << std::endl;
        // Продолжить очистку временной папки, даже если не удалось сохранить
    }

    bool save_success = false;
    if (!ec_dir)
    { // Только если удалось создать выходной каталог
        save_success = compositeImage.save(QString::fromStdString(output_file_name), "BMP");
    }

    std::error_code ec_cleanup;
    std::filesystem::remove_all(temp_dir_path, ec_cleanup);
    if (ec_cleanup)
    {
        std::cerr << "Ошибка при очистке временного каталога '" << temp_dir_path << "' (filesystem): " << ec_cleanup.message() << std::endl;
    }
    else
    {
        std::cout << "Временный каталог '" << temp_dir_path << "' очищен." << std::endl;
    }

    if (save_success)
    {
        std::cout << "Композитный скриншот для объекта " << object_name_identifier << " сохранен: " << output_file_name << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Ошибка сохранения композитного скриншота для объекта " << object_name_identifier << ": " << output_file_name << std::endl;
        return false;
    }
}

// В capturethread.cpp, перед функцией combineAndCleanupScreenshots или в начале файла
const std::string screen_temp_directory_name_base = "screen_temp";

// ... (определение функции combineAndCleanupScreenshots) ...

CaptureThread::CaptureThread(std::vector<MapObject> objects,
                             int interval,
                             std::string st_time,
                             std::string en_time,
                             QObject *parent)
    : QThread(parent),
      m_mapObjects(std::move(objects)),
      m_capture_interval_sec(interval),
      m_start_time_str(std::move(st_time)),
      m_end_time_str(std::move(en_time)),
      running(false) {}

void CaptureThread::stop()
{
    running = false;
}

void CaptureThread::run()
{
    running = true;
    std::cout << "Поток захвата запущен." << std::endl;

    while (running)
    {
        std::time_t now_t = std::time(nullptr);
        std::tm *current_time_tm = std::localtime(&now_t);

        if (!current_time_tm)
        {
            std::cerr << "Ошибка получения текущего времени. Пропускаем цикл захвата." << std::endl;
            sleepAndCheckRunning(10);
            continue;
        }

        if (!isWithinCaptureTimeWindow(current_time_tm))
        {
            sleepAndCheckRunning(300);
            continue;
        }

        std::cout << "Внутри времени захвата. Запуск обработки объектов (" << m_mapObjects.size() << " шт.)." << std::endl;

        for (size_t i = 0; i < m_mapObjects.size() && running; ++i)
        {
            const auto &mapObject = m_mapObjects[i];
            std::cout << "Обработка объекта: " << mapObject.name << std::endl;
            std::vector<std::pair<double, double>> current_object_coords;
            generateCoordinatesForObject(mapObject, current_object_coords);

            if (current_object_coords.empty())
            {
                std::cerr << "Нет координат для объекта " << mapObject.name << ". Пропуск." << std::endl;
                continue;
            }

            std::string object_temp_dir_name = screen_temp_directory_name_base + "_" + mapObject.name;
            std::string object_temp_full_path = mapObject.save_directory + "/" + object_temp_dir_name;

            std::error_code ec;
            std::filesystem::remove_all(object_temp_full_path, ec);
            if (ec)
            {
                std::cerr << "Ошибка при очистке временного каталога объекта " << mapObject.name << ": " << ec.message() << std::endl;
            }

            std::filesystem::create_directories(object_temp_full_path, ec);
            if (ec)
            {
                std::cerr << "Ошибка создания временного каталога для объекта " << mapObject.name << ": " << object_temp_full_path << " - " << ec.message() << std::endl;
                continue;
            }

            bool capture_loop_completed_for_object = true;
            for (size_t u = 0; u < current_object_coords.size() && running; ++u)
            {
                std::time_t snap_time_t = std::time(nullptr);
                std::tm *snap_time_tm = std::localtime(&snap_time_t);
                createSnapshot(current_object_coords[u], object_temp_full_path, "Скриншот_%Y-%m-%d_%H-%M-%S", static_cast<int>(u), snap_time_tm);
            }

            if (!running)
            {
                capture_loop_completed_for_object = false;
                std::cout << "Запрошена остановка во время захвата для объекта " << mapObject.name << std::endl;
            }

            if (capture_loop_completed_for_object)
            {
                now_t = std::time(nullptr);
                current_time_tm = std::localtime(&now_t);
                combineAndCleanupScreenshots(object_temp_full_path, mapObject.save_directory, current_time_tm, mapObject.name);
            }
            else
            {
                std::cerr << "Захват для объекта " << mapObject.name << " прерван. Очистка временных файлов." << std::endl;
                std::filesystem::remove_all(object_temp_full_path, ec);
                if (ec)
                {
                    std::cerr << "Ошибка при очистке частичного временного каталога объекта " << mapObject.name << ": " << ec.message() << std::endl;
                }
            }

            if (!running)
                break;
        }

        if (running)
        {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm *local_tm = std::localtime(&now_c);

            if (m_capture_interval_sec > 0)
            {
                int seconds_to_wait = m_capture_interval_sec;
                if (m_capture_interval_sec % 3600 == 0 && local_tm && local_tm->tm_min > 1)
                {
                    int minutes_to_next_hour = 60 - local_tm->tm_min;
                    seconds_to_wait = minutes_to_next_hour * 60 + 60;
                }
                sleepAndCheckRunning(seconds_to_wait);
            }
            else
            {
                sleepAndCheckRunning(60);
            }
        }
    } // <-- Закрывающая скобка для CaptureThread::run()
    // Здесь должен быть конец CaptureThread::run(),
    // а остальные методы должны быть ниже, вне этой функции.
}

// Реализации методов класса CaptureThread должны идти здесь:
void CaptureThread::sleepAndCheckRunning(int seconds) // Убедитесь, что здесь есть "CaptureThread::"
{
    for (int i = 0; i < seconds * 10 && running; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool CaptureThread::isWithinCaptureTimeWindow(const std::tm *current_time_tm) // Убедитесь, что здесь есть "CaptureThread::"
{
    if (!current_time_tm)
        return false;
    size_t start_colon = m_start_time_str.find(':');
    size_t end_colon = m_end_time_str.find(':');
    if (start_colon == std::string::npos || end_colon == std::string::npos || m_start_time_str.length() < 4 || m_end_time_str.length() < 4)
    {
        std::cerr << "Неверный формат времени в настройках. Используйте hh:mm." << std::endl;
        return false;
    }
    try
    {
        int start_hour = std::stoi(m_start_time_str.substr(0, start_colon));
        int start_minute = std::stoi(m_start_time_str.substr(start_colon + 1));
        int end_hour = std::stoi(m_end_time_str.substr(0, end_colon));
        int end_minute = std::stoi(m_end_time_str.substr(end_colon + 1));
        if (start_hour < 0 || start_hour > 23 || start_minute < 0 || start_minute > 59 ||
            end_hour < 0 || end_hour > 23 || end_minute < 0 || end_minute > 59)
        {
            std::cerr << "Неверное значение часа или минуты в диапазоне времени." << std::endl;
            return false;
        }
        int current_total_minutes = current_time_tm->tm_hour * 60 + current_time_tm->tm_min;
        int start_total_minutes = start_hour * 60 + start_minute;
        int end_total_minutes = end_hour * 60 + end_minute;
        if (start_total_minutes <= end_total_minutes)
        {
            return current_total_minutes >= start_total_minutes && current_total_minutes < end_total_minutes;
        }
        else
        {
            return current_total_minutes >= start_total_minutes || current_total_minutes < end_total_minutes;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка парсинга времени: " << e.what() << std::endl;
        return false;
    }
    return false;
}

void CaptureThread::generateCoordinatesForObject(const MapObject &obj, std::vector<std::pair<double, double>> &out_coords) // Убедитесь, что здесь есть "CaptureThread::"
{
    out_coords.clear();
    double lat0 = obj.latitude_center;
    double lon0 = obj.longitude_center;
    double r_km = static_cast<double>(obj.radius_km);
    const double deg_per_km_at_equator = 0.01;
    const double step_deg_y = 0.0137;
    const double step_deg_x = 0.0193;
    double span_from_center_lat_deg = r_km * deg_per_km_at_equator;
    double cos_lat0 = std::cos(lat0 * M_PI / 180.0);
    if (std::abs(cos_lat0) < std::numeric_limits<double>::epsilon())
    {
        cos_lat0 = std::numeric_limits<double>::epsilon();
    }
    double span_from_center_lon_deg = r_km * deg_per_km_at_equator / cos_lat0;
    int num_steps_from_center_lat = static_cast<int>(std::floor(span_from_center_lat_deg / step_deg_y));
    int num_steps_from_center_lon = static_cast<int>(std::floor(span_from_center_lon_deg / step_deg_x));
    int N_grid = std::max(1, 2 * std::max(num_steps_from_center_lat, num_steps_from_center_lon) + 1);
    if (N_grid == 0)
        N_grid = 1;
    double start_lat = lat0 - (N_grid - 1) / 2.0 * step_deg_y - step_deg_y / 2.0;
    double start_lon = lon0 - (N_grid - 1) / 2.0 * step_deg_x - step_deg_x / 2.0;
    std::cout << "Для объекта '" << obj.name << "': Центр(" << lat0 << ", " << lon0 << "), R=" << r_km
              << "км. Сетка " << N_grid << "x" << N_grid << "." << std::endl;
    for (int yy = 0; yy < N_grid; ++yy)
    {
        for (int xx = 0; xx < N_grid; ++xx)
        {
            double current_tile_lat_bottom = start_lat + yy * step_deg_y;
            double current_tile_lon_left = start_lon + xx * step_deg_x;
            out_coords.push_back({current_tile_lat_bottom, current_tile_lon_left});
        }
    }
}

void CaptureThread::createSnapshot(std::pair<double, double> bottom_left_coord, const std::string &directory, const std::string &format, int index, std::tm *current_time_tm) // Убедитесь, что здесь есть "CaptureThread::"
{
    if (!running)
        return;
    char filename_time_buffer[80];
    std::strftime(filename_time_buffer, sizeof(filename_time_buffer), format.c_str(), current_time_tm);
    std::string file_name = directory + "/" + filename_time_buffer + "_" + std::to_string(index) + ".png";
    if (!std::filesystem::exists(directory))
    {
        std::cerr << "Целевой каталог для снимка не существует: " << directory << ". Пропускаем снимок." << std::endl;
        return;
    }
    double lat_bottom = bottom_left_coord.first;
    double lon_left = bottom_left_coord.second;
    const double tile_height_deg = 0.01;
    const double tile_width_deg = 0.01;
    double lon_right = lon_left + tile_width_deg;
    double lat_top = lat_bottom + tile_height_deg;
    std::ostringstream oss_api_url;
    oss_api_url.imbue(std::locale("C"));
    oss_api_url << "https://static-maps.yandex.ru/1.x/?bbox="
                << lon_left << "," << lat_bottom << "~" << lon_right << "," << lat_top
                << "&size=450,450&l=map,trf";
    std::string api_url = oss_api_url.str();
    CURL *curl = curl_easy_init();
    if (curl)
    {
        FILE *file = fopen(file_name.c_str(), "wb");
        if (file)
        {
            curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
            CURLcode res = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            fclose(file);
            if (res == CURLE_OK && http_code >= 200 && http_code < 300)
            {
                std::ifstream ifs(file_name, std::ios::binary | std::ios::ate);
                if (!ifs.is_open() || ifs.tellg() == 0)
                {
                    std::cerr << "Ошибка: Загруженный файл пуст или не удалось проверить: " << file_name << std::endl;
                    std::filesystem::remove(file_name);
                }
                else
                {
                    std::cout << "Снимок сохранен: " << file_name << std::endl; // Слишком много логов
                    // QString info = QString("Снимок сохранен: %1")
                    //                    .arg(QString::fromStdString(file_name));

                    // updateStatistics(info);
                }
            }
            else
            {
                std::cerr << "Ошибка CURL (код: " << res << ", HTTP: " << http_code << ") для URL " << api_url << ": " << curl_easy_strerror(res) << std::endl;
                std::filesystem::remove(file_name);
            }
        }
        else
        {
            std::cerr << "Ошибка открытия файла для записи: " << file_name << " (errno: " << errno << ")" << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    else
    {
        std::cerr << "Ошибка инициализации CURL." << std::endl;
    }
}
