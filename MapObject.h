#ifndef MAPOBJECT_H
#define MAPOBJECT_H

#include <string>
#include <QString> // Для getDisplayText

class MapObject
{
public:
    double latitude_center;
    double longitude_center;
    int radius_km;
    std::string name;           // Уникальное имя объекта для файлов и логов
    std::string save_directory; // Каталог для сохранения снимков этого объекта

    MapObject(double lat, double lon, int rad_km, std::string obj_name, std::string save_dir);

    QString getDisplayText() const;
};

#endif // MAPOBJECT_H
