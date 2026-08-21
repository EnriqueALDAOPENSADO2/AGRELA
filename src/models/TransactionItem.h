#pragma once

#include <QString>
#include <QDate>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include "Invoice.h"

// Registro de una Venta realizada (a partir de una factura emitida)
struct SaleRecord {
    QString id;
    QString numeroFactura;
    QDate fecha;
    Customer cliente;
    double baseImponible = 0.0;
    double cuotaIva = 0.0;
    double totalFactura = 0.0;
    QString excelPath;
    QString pdfPath;
    int numArticulos = 0;

    static SaleRecord fromInvoice(const Invoice& inv, const QString& xlsxPath = "", const QString& pdfPath = "") {
        SaleRecord rec;
        rec.id = inv.numeroFactura + "_" + inv.fecha.toString("yyyyMMdd");
        rec.numeroFactura = inv.numeroFactura;
        rec.fecha = inv.fecha;
        rec.cliente = inv.cliente;
        rec.baseImponible = std::round(inv.calcularTotalBruto() * 100.0) / 100.0;
        rec.cuotaIva = std::round(inv.calcularCuotaIva() * 100.0) / 100.0;
        rec.totalFactura = std::round(inv.calcularTotalFactura() * 100.0) / 100.0;
        rec.excelPath = xlsxPath;
        rec.pdfPath = pdfPath;
        rec.numArticulos = inv.items.size();
        return rec;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["numero_factura"] = numeroFactura;
        obj["fecha"] = fecha.toString("yyyy-MM-dd");
        obj["cliente_nombre"] = cliente.nombre;
        obj["cliente_cif"] = cliente.cifNif;
        obj["base_imponible"] = std::round(baseImponible * 100.0) / 100.0;
        obj["cuota_iva"] = std::round(cuotaIva * 100.0) / 100.0;
        obj["total_factura"] = std::round(totalFactura * 100.0) / 100.0;
        obj["excel_path"] = excelPath;
        obj["pdf_path"] = pdfPath;
        obj["num_articulos"] = numArticulos;
        return obj;
    }

    static SaleRecord fromJson(const QJsonObject& obj) {
        SaleRecord rec;
        rec.id = obj["id"].toString();
        rec.numeroFactura = obj["numero_factura"].toString();
        rec.fecha = QDate::fromString(obj["fecha"].toString(), "yyyy-MM-dd");
        rec.cliente.nombre = obj["cliente_nombre"].toString();
        rec.cliente.cifNif = obj["cliente_cif"].toString();
        rec.baseImponible = std::round(obj["base_imponible"].toDouble(0.0) * 100.0) / 100.0;
        rec.cuotaIva = std::round(obj["cuota_iva"].toDouble(0.0) * 100.0) / 100.0;
        rec.totalFactura = std::round(obj["total_factura"].toDouble(0.0) * 100.0) / 100.0;
        rec.excelPath = obj["excel_path"].toString();
        rec.pdfPath = obj["pdf_path"].toString();
        rec.numArticulos = obj["num_articulos"].toInt(0);
        return rec;
    }
};

// Registro de una Compra a Proveedor
struct PurchaseRecord {
    QString id;
    QDate fecha;
    QString proveedor;
    QString cifNif;
    QString numFacturaProveedor;
    QString concepto;
    double unidades = 1.0;
    double precioUnitario = 0.0;
    double tipoIva = 0.21; // 0.21 por defecto

    double calcularBase() const {
        return std::round((unidades * precioUnitario) * 100.0) / 100.0;
    }

    double calcularIva() const {
        return std::round((calcularBase() * tipoIva) * 100.0) / 100.0;
    }

    double calcularTotal() const {
        return std::round((calcularBase() + calcularIva()) * 100.0) / 100.0;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["fecha"] = fecha.toString("yyyy-MM-dd");
        obj["proveedor"] = proveedor;
        obj["cif_nif"] = cifNif;
        obj["num_factura_proveedor"] = numFacturaProveedor;
        obj["concepto"] = concepto;
        obj["unidades"] = unidades;
        obj["precio_unitario"] = std::round(precioUnitario * 100.0) / 100.0;
        obj["tipo_iva"] = tipoIva;
        obj["base_imponible"] = calcularBase();
        obj["cuota_iva"] = calcularIva();
        obj["total"] = calcularTotal();
        return obj;
    }

    static PurchaseRecord fromJson(const QJsonObject& obj) {
        PurchaseRecord rec;
        rec.id = obj["id"].toString();
        rec.fecha = QDate::fromString(obj["fecha"].toString(), "yyyy-MM-dd");
        rec.proveedor = obj["proveedor"].toString();
        rec.cifNif = obj["cif_nif"].toString();
        rec.numFacturaProveedor = obj["num_factura_proveedor"].toString();
        rec.concepto = obj["concepto"].toString();
        rec.unidades = obj["unidades"].toDouble(1.0);
        rec.precioUnitario = std::round(obj["precio_unitario"].toDouble(0.0) * 100.0) / 100.0;
        rec.tipoIva = obj["tipo_iva"].toDouble(0.21);
        return rec;
    }
};
