#include "Invoice.h"
#include <cmath>

Customer Customer::fromJson(const QJsonObject& obj) {
    Customer c;
    c.alias = obj["alias"].toString();
    c.nombre = obj["nombre"].toString();
    c.cifNif = obj["cif_nif"].toString();
    c.direccion = obj["direccion"].toString();
    c.poblacion = obj["poblacion"].toString();
    c.provincia = obj["provincia"].toString();
    c.telefono = obj["telefono"].toString();
    c.email = obj["email"].toString();
    c.file = obj["file"].toString();
    return c;
}

QJsonObject Customer::toJson() const {
    QJsonObject obj;
    obj["alias"] = alias;
    obj["nombre"] = nombre;
    obj["cif_nif"] = cifNif;
    obj["direccion"] = direccion;
    obj["poblacion"] = poblacion;
    obj["provincia"] = provincia;
    obj["telefono"] = telefono;
    obj["email"] = email;
    obj["file"] = file;
    return obj;
}

double Invoice::calcularTotalBruto() const {
    double total = 0.0;
    for (const auto& item : items) {
        total += item.calcularTotal();
    }
    return std::round(total * 100.0) / 100.0;
}

double Invoice::calcularCuotaIva() const {
    double rawIva = calcularTotalBruto() * tipoIva;
    return std::round(rawIva * 100.0) / 100.0;
}

double Invoice::calcularTotalFactura() const {
    double rawTotal = calcularTotalBruto() + calcularCuotaIva();
    return std::round(rawTotal * 100.0) / 100.0;
}

Invoice Invoice::fromJson(const QJsonObject& obj) {
    Invoice inv;
    inv.emisorNombre = obj["emisor_nombre"].toString(inv.emisorNombre);
    inv.emisorDireccion = obj["emisor_direccion"].toString(inv.emisorDireccion);
    inv.emisorPoligono = obj["emisor_poligono"].toString(inv.emisorPoligono);
    inv.emisorCp = obj["emisor_cp"].toString(inv.emisorCp);
    inv.emisorCiudad = obj["emisor_ciudad"].toString(inv.emisorCiudad);
    inv.emisorCif = obj["emisor_cif"].toString(inv.emisorCif);

    if (obj.contains("cliente")) {
        inv.cliente = Customer::fromJson(obj["cliente"].toObject());
    }

    inv.numeroFactura = obj["numero_factura"].toString("1");
    inv.fecha = QDate::fromString(obj["fecha"].toString(), Qt::ISODate);
    if (!inv.fecha.isValid()) inv.fecha = QDate::currentDate();
    inv.formaPago = obj["forma_pago"].toString("TPV");
    inv.tipoIva = obj["tipo_iva"].toDouble(0.21);

    inv.items.clear();
    QJsonArray arr = obj["items"].toArray();
    for (const auto& val : arr) {
        inv.items.append(InvoiceItem::fromJson(val.toObject()));
    }
    return inv;
}

QJsonObject Invoice::toJson() const {
    QJsonObject obj;
    obj["emisor_nombre"] = emisorNombre;
    obj["emisor_direccion"] = emisorDireccion;
    obj["emisor_poligono"] = emisorPoligono;
    obj["emisor_cp"] = emisorCp;
    obj["emisor_ciudad"] = emisorCiudad;
    obj["emisor_cif"] = emisorCif;

    obj["cliente"] = cliente.toJson();
    obj["numero_factura"] = numeroFactura;
    obj["fecha"] = fecha.toString(Qt::ISODate);
    obj["forma_pago"] = formaPago;
    obj["tipo_iva"] = tipoIva;

    QJsonArray arr;
    for (const auto& item : items) {
        arr.append(item.toJson());
    }
    obj["items"] = arr;

    obj["total_bruto"] = calcularTotalBruto();
    obj["cuota_iva"] = calcularCuotaIva();
    obj["total_factura"] = calcularTotalFactura();
    return obj;
}
