#include <QApplication>
#include "snapshotapp.h" // Включите заголовочный файл SnapshotApp

// Класс MapObject оставлен здесь для простоты, но его также можно вынести
// class MapObject
// {
// public:
//     double latitude_center;
//     double longitude_center;
//     int radius_km;
//     std::string name;           // Уникальное имя объекта для файлов и логов
//     std::string save_directory; // Каталог для сохранения снимков этого объекта

//     MapObject(double lat, double lon, int rad_km, std::string obj_name, std::string save_dir)
//         : latitude_center(lat), longitude_center(lon), radius_km(rad_km), name(std::move(obj_name)), save_directory(std::move(save_dir)) {}

//     QString getDisplayText() const
//     {
//         return QString("Имя: %1, Центр: (%2, %3), Радиус: %4 км, Путь: %5")
//             .arg(QString::fromStdString(name))
//             .arg(latitude_center)
//             .arg(longitude_center)
//             .arg(radius_km)
//             .arg(QString::fromStdString(save_directory));
//     }
// };


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    std::locale::global(std::locale("C")); // Для корректного преобразования чисел в строки (точка как разделитель)

    std::string base_screenshot_dir = "./screenshots_output";
    std::error_code ec;
    if (!std::filesystem::exists(base_screenshot_dir))
    {
        std::filesystem::create_directories(base_screenshot_dir, ec);
        if (ec)
        {
            std::cerr << "Предупреждение: Не удалось создать базовый каталог для скриншотов '" << base_screenshot_dir << "': " << ec.message() << std::endl;
        }
    }

    SnapshotApp window;
    window.resize(500, 750);
    window.show();

    return app.exec();
}
