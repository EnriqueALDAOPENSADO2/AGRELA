#pragma once

#include <QString>
#include <QJsonObject>
#include "CatalogItem.h"

struct InvoiceItem {
    QString code;
    QString desc;
    double unidades = 1.0;
    double precioUnitario = 0.0;
    double anchoPersianaFinal = 0.0; // mm (se muestra en la factura al cliente)
    double anchoRolloUsado = 0.0;     // mm (ancho del rollo/bobina para calcular M² y coste)
    double alto = 0.0;               // mm (se muestra en la factura)
    QString unidad = "ud.";          // "ud.", "ml.", "m²"
    QString imgPath;
    bool aplicarMinimoCompacto = false; // mínimo 1.50 m² para compactos si se activa

    // Superficie de 1 unidad individual en m²
    double calcularMetrosCuadradosUnitario() const;
    
    // Superficie total facturada (unidades * superficie unitaria) en m²
    double calcularMetrosCuadrados() const;
    
    // Total importe de la línea (€)
    double calcularTotal() const;

    static InvoiceItem fromCatalogItem(const CatalogItem& catItem);
    static InvoiceItem fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
};
