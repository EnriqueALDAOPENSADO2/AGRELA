#pragma once

#include <QString>
#include <QPair>
#include "../models/Invoice.h"

class InvoiceGeneratorService {
public:
    static InvoiceGeneratorService& instance();

    // Genera tanto el archivo Excel (.xlsx) como el PDF (.pdf) simultáneamente
    // Retorna un par con las rutas: <ruta_excel, ruta_pdf>
    QPair<QString, QString> generateBoth(const Invoice& invoice, const QString& targetDir = "facturas");

    bool generateExcel(const Invoice& invoice, const QString& outputXlsxPath);
    bool generatePdf(const Invoice& invoice, const QString& outputPdfPath);

private:
    InvoiceGeneratorService() = default;
    QString sanitizeFilename(const QString& name) const;
};
