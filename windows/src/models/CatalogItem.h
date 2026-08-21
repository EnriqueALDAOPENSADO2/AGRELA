#pragma once

#include <QString>
#include <QJsonObject>

struct CatalogItem {
    QString sheet;
    QString category;
    QString code;
    QString desc;
    double p1 = 0.0;
    QString u1;
    double p2 = 0.0;
    QString u2;
    QString imgPath;

    static CatalogItem fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
};
