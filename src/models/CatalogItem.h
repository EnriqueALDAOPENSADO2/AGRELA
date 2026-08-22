#pragma once

#include <QString>
#include <QJsonObject>

struct CatalogItem {
    QString sheet;
    QString category;
    QString code;
    QString desc;
    double pvp = 0.0;
    double t1 = 0.0;
    double p1 = 0.0;     // Compatibilidad: equivale a PVP
    double p_t1 = 0.0;   // Compatibilidad: equivale a T1
    QString u1;
    double p2 = 0.0;
    QString u2;
    QString imgPath;

    double getPriceForTariff(const QString& tariff) const {
        if (tariff.compare("T1", Qt::CaseInsensitive) == 0 || 
            tariff.compare("T-1", Qt::CaseInsensitive) == 0 ||
            tariff.compare("Tarifa 1", Qt::CaseInsensitive) == 0) {
            return (t1 > 0.0) ? t1 : ((p_t1 > 0.0) ? p_t1 : p1);
        }
        return (pvp > 0.0) ? pvp : p1;
    }

    static CatalogItem fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
};
