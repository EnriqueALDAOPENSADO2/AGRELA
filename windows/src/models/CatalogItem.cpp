#include "CatalogItem.h"

CatalogItem CatalogItem::fromJson(const QJsonObject& obj) {
    CatalogItem item;
    item.sheet = obj["sheet"].toString();
    item.category = obj["category"].toString();
    item.code = obj["code"].toString();
    item.desc = obj["desc"].toString();
    item.p1 = obj["p1"].toDouble(0.0);
    item.u1 = obj["u1"].toString();
    item.p2 = obj["p2"].toDouble(0.0);
    item.u2 = obj["u2"].toString();
    item.imgPath = obj["img_path"].toString();
    return item;
}

QJsonObject CatalogItem::toJson() const {
    QJsonObject obj;
    obj["sheet"] = sheet;
    obj["category"] = category;
    obj["code"] = code;
    obj["desc"] = desc;
    obj["p1"] = p1;
    obj["u1"] = u1;
    obj["p2"] = p2;
    obj["u2"] = u2;
    obj["img_path"] = imgPath;
    return obj;
}
