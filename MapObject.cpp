#include "MapObject.h"

MapObject::MapObject(double lat, double lon, int rad_km, std::string obj_name, std::string save_dir)
    : latitude_center(lat), longitude_center(lon), radius_km(rad_km), name(std::move(obj_name)), save_directory(std::move(save_dir)) {}

QString MapObject::getDisplayText() const
{
    return QString("Имя: %1, Центр: (%2, %3), Радиус: %4 км, Путь: %5")
        .arg(QString::fromStdString(name))
        .arg(latitude_center)
        .arg(longitude_center)
        .arg(radius_km)
        .arg(QString::fromStdString(save_directory));
}
