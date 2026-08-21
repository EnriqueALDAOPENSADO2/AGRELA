#include "InvoiceItem.h"
#include <algorithm>
#include <cmath>

double InvoiceItem::calcularMetrosCuadradosUnitario() const {
    double w = (anchoRolloUsado > 0.0) ? anchoRolloUsado : anchoPersianaFinal;
    if (w > 0.0 && alto > 0.0) {
        double m2 = (w / 1000.0) * (alto / 1000.0);
        if (aplicarMinimoCompacto && m2 < 1.50) {
            return 1.50;
        }
        return m2;
    }
    if (w > 0.0 && alto <= 0.0) {
        // En caso de perfiles/tubos lineales (ml)
        return (w / 1000.0);
    }
    return 0.0;
}

double InvoiceItem::calcularMetrosCuadrados() const {
    double unitM2 = calcularMetrosCuadradosUnitario();
    if (unitM2 > 0.0) {
        return unidades * unitM2;
    }
    return 0.0;
}

double InvoiceItem::calcularTotal() const {
    double totalM2 = calcularMetrosCuadrados();
    double rawTotal = 0.0;
    if (totalM2 > 0.0) {
        // Para persianas por m² y perfiles por ml calculados por dimensión
        rawTotal = totalM2 * precioUnitario;
    } else {
        // Para productos por unidad fija
        rawTotal = unidades * precioUnitario;
    }
    return std::round(rawTotal * 100.0) / 100.0;
}

InvoiceItem InvoiceItem::fromCatalogItem(const CatalogItem& catItem) {
    InvoiceItem item;
    item.code = catItem.code;
    item.desc = catItem.desc;
    item.unidades = 1.0;
    item.precioUnitario = catItem.p1;
    item.unidad = catItem.u1.isEmpty() ? "ud." : catItem.u1;
    item.imgPath = catItem.imgPath;
    item.aplicarMinimoCompacto = catItem.sheet.contains("Compactos", Qt::CaseInsensitive);
    return item;
}

InvoiceItem InvoiceItem::fromJson(const QJsonObject& obj) {
    InvoiceItem item;
    item.code = obj["code"].toString();
    item.desc = obj["desc"].toString();
    item.unidades = obj["unidades"].toDouble(1.0);
    item.precioUnitario = obj["precio_unitario"].toDouble(0.0);
    item.anchoPersianaFinal = obj["ancho_persiana_final"].toDouble(0.0);
    item.anchoRolloUsado = obj["ancho_rollo_usado"].toDouble(0.0);
    item.alto = obj["alto"].toDouble(0.0);
    item.unidad = obj["unidad"].toString("ud.");
    item.imgPath = obj["img_path"].toString();
    item.aplicarMinimoCompacto = obj["aplicar_minimo_compacto"].toBool(false);
    return item;
}

QJsonObject InvoiceItem::toJson() const {
    QJsonObject obj;
    obj["code"] = code;
    obj["desc"] = desc;
    obj["unidades"] = unidades;
    obj["precio_unitario"] = precioUnitario;
    obj["ancho_persiana_final"] = anchoPersianaFinal;
    obj["ancho_rollo_usado"] = anchoRolloUsado;
    obj["alto"] = alto;
    obj["unidad"] = unidad;
    obj["img_path"] = imgPath;
    obj["aplicar_minimo_compacto"] = aplicarMinimoCompacto;
    return obj;
}
