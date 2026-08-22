#pragma once

#include <QString>
#include <QDate>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include "InvoiceItem.h"

struct Customer {
    QString alias;      // Nombre del archivo / Alias (ej: "VENCORIS")
    QString nombre;     // Razón social / Nombre fiscal (ej: "VENTANAS CORISTANCO S. L.")
    QString cifNif;
    QString direccion;
    QString poblacion;
    QString provincia;
    QString telefono;
    QString email;
    QString file;

    static Customer fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
};

struct Invoice {
    // Datos Empresa Emisora
    QString emisorNombre = "Juan Manuel Aldao López";
    QString emisorDireccion = "C/ Gutemberg Nº 44-A";
    QString emisorPoligono = "Polígono A Grela";
    QString emisorCp = "C.P. 15008";
    QString emisorCiudad = "A Coruña";
    QString emisorCif = "D.N.I. 52434449-S";

    // Datos Cliente
    Customer cliente;

    // Datos Factura
    QString numeroFactura = "1";
    QDate fecha = QDate::currentDate();
    QDate fechaVencimiento = QDate::currentDate();
    QString formaPago = "TPV";
    QString tarifa = "PVP"; // "PVP" o "T1"

    // Líneas de artículos
    QVector<InvoiceItem> items;

    // Impuestos
    double tipoIva = 0.21; // 21%

    // Métodos de cálculo
    double calcularTotalBruto() const;
    double calcularCuotaIva() const;
    double calcularTotalFactura() const;

    static Invoice fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
};
