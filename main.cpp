#include <QApplication>
#include "snapshotapp.h" // Включите заголовочный файл SnapshotApp

std::string base_screenshot_dir = "./screenshots_output";

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    std::locale::global(std::locale("C")); // Для корректного преобразования чисел в строки (точка как разделитель)

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
    window.resize(700, 750);
    window.show();

    return app.exec();
}
