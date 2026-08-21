#include "InvoiceGeneratorService.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QPrinter>
#include <QPainter>
#include <QPixmap>
#include <QFont>
#include <QFontMetrics>
#include <QPen>
#include <QBrush>
#include <QDateTime>
#include <QCoreApplication>
#include <QDebug>

InvoiceGeneratorService& InvoiceGeneratorService::instance() {
    static InvoiceGeneratorService inst;
    return inst;
}

QString InvoiceGeneratorService::sanitizeFilename(const QString& name) const {
    QString clean = name.trimmed();
    clean.replace(" ", "_");
    clean.replace("/", "_");
    clean.replace("\\", "_");
    clean.replace(":", "_");
    clean.replace("*", "_");
    clean.replace("?", "_");
    clean.replace("\"", "_");
    clean.replace("<", "_");
    clean.replace(">", "_");
    clean.replace("|", "_");
    if (clean.isEmpty()) clean = "Cliente";
    return clean;
}

QPair<QString, QString> InvoiceGeneratorService::generateBoth(const Invoice& invoice, const QString& targetDir) {
    QDir dir(targetDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString numStr = invoice.numeroFactura.trimmed();
    if (numStr.isEmpty()) numStr = "1";
    QString clientStr = sanitizeFilename(invoice.cliente.nombre);
    QString dateStr = invoice.fecha.toString("yyyyMMdd");
    
    QString baseName = QString("Factura_%1_%2_%3").arg(numStr, clientStr, dateStr);
    QString xlsxPath = dir.filePath(baseName + ".xlsx");
    QString pdfPath = dir.filePath(baseName + ".pdf");

    bool okXlsx = generateExcel(invoice, xlsxPath);
    bool okPdf = generatePdf(invoice, pdfPath);

    qDebug() << "Generación simultánea terminada:"
             << "Excel:" << (okXlsx ? xlsxPath : "ERROR")
             << "PDF:" << (okPdf ? pdfPath : "ERROR");

    return qMakePair(okXlsx ? xlsxPath : QString(), okPdf ? pdfPath : QString());
}

extern "C" {
#include "miniz.h"
}
#include <sstream>

static std::string xmlEscape(const QString& qstr) {
    std::string str = qstr.toStdString();
    std::string out;
    out.reserve(str.size() + 10);
    for (char c : str) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '\"': out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default: out += c; break;
        }
    }
    return out;
}

static std::string buildSheet1Xml(const Invoice& invoice) {
    std::ostringstream oss;
    oss << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    oss << "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">\n";
    oss << "<sheetPr><pageSetUpPr fitToPage=\"1\"/></sheetPr>\n";
    oss << "<dimension ref=\"A1:Q50\"/>\n";
    oss << "<sheetViews><sheetView showGridLines=\"0\" showRowColHeaders=\"0\" showZeros=\"0\" tabSelected=\"1\" workbookViewId=\"0\"/></sheetViews>\n";
    oss << "<sheetFormatPr defaultColWidth=\"8.7\" defaultRowHeight=\"13\"/>\n";
    oss << "<cols>\n";
    oss << "<col min=\"1\" max=\"1\" width=\"4\" customWidth=\"1\"/>\n";
    oss << "<col min=\"2\" max=\"2\" width=\"38\" customWidth=\"1\"/>\n";
    oss << "<col min=\"3\" max=\"3\" width=\"28\" customWidth=\"1\"/>\n";
    oss << "<col min=\"4\" max=\"4\" width=\"18\" customWidth=\"1\"/>\n";
    oss << "<col min=\"5\" max=\"9\" width=\"14\" customWidth=\"1\"/>\n";
    oss << "<col min=\"10\" max=\"10\" width=\"18\" customWidth=\"1\"/>\n";
    oss << "</cols>\n";
    oss << "<sheetData>\n";

    // Row 2: Title
    oss << "<row r=\"2\"><c r=\"B2\" t=\"inlineStr\"><is><t>FACTURA</t></is></c><c r=\"E2\" t=\"inlineStr\"><is><t>DATOS DEL CLIENTE</t></is></c></row>\n";

    // Row 4: Company Header
    oss << "<row r=\"4\"><c r=\"B4\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.emisorNombre) << "</t></is></c></row>\n";

    // Row 5: Address / Client Name
    oss << "<row r=\"5\"><c r=\"B5\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.emisorDireccion) << "</t></is></c>";
    oss << "<c r=\"E5\" t=\"inlineStr\"><is><t>Nombre</t></is></c>";
    oss << "<c r=\"F5\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.cliente.nombre) << "</t></is></c></row>\n";

    // Row 6: Poligono / Client Address
    oss << "<row r=\"6\"><c r=\"B6\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.emisorPoligono) << "</t></is></c>";
    oss << "<c r=\"E6\" t=\"inlineStr\"><is><t>Dirección</t></is></c>";
    oss << "<c r=\"F6\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.cliente.direccion) << "</t></is></c></row>\n";

    // Row 7: CP / Client Poblacion
    oss << "<row r=\"7\"><c r=\"B7\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.emisorCp) << "</t></is></c>";
    oss << "<c r=\"E7\" t=\"inlineStr\"><is><t>Población</t></is></c>";
    oss << "<c r=\"F7\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.cliente.poblacion) << "</t></is></c></row>\n";

    // Row 8: Ciudad / Client Provincia
    oss << "<row r=\"8\"><c r=\"B8\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.emisorCiudad) << "</t></is></c>";
    oss << "<c r=\"E8\" t=\"inlineStr\"><is><t>Provincia</t></is></c>";
    oss << "<c r=\"F8\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.cliente.provincia) << "</t></is></c></row>\n";

    // Row 9: CIF / Client CIF
    oss << "<row r=\"9\"><c r=\"B9\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.emisorCif) << "</t></is></c>";
    oss << "<c r=\"E9\" t=\"inlineStr\"><is><t>CIF/NIF</t></is></c>";
    oss << "<c r=\"F9\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.cliente.cifNif) << "</t></is></c></row>\n";

    // Row 11: Factura Nº, Fecha
    oss << "<row r=\"11\"><c r=\"E11\" t=\"inlineStr\"><is><t>Factura</t></is></c>";
    oss << "<c r=\"F11\" t=\"inlineStr\"><is><t>Nº " << xmlEscape(invoice.numeroFactura) << "</t></is></c>";
    oss << "<c r=\"G11\" t=\"inlineStr\"><is><t>Fecha</t></is></c>";
    oss << "<c r=\"H11\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.fecha.toString("dd/MM/yyyy")) << "</t></is></c></row>\n";

    // Row 12: Column headers
    oss << "<row r=\"12\"><c r=\"C12\" t=\"inlineStr\"><is><t>DESCRIPCION</t></is></c>";
    oss << "<c r=\"E12\" t=\"inlineStr\"><is><t>Uds.</t></is></c>";
    oss << "<c r=\"F12\" t=\"inlineStr\"><is><t>PRECIO</t></is></c>";
    oss << "<c r=\"G12\" t=\"inlineStr\"><is><t>ANCHO (mm)</t></is></c>";
    oss << "<c r=\"H12\" t=\"inlineStr\"><is><t>ALTO (mm)</t></is></c>";
    oss << "<c r=\"I12\" t=\"inlineStr\"><is><t>M²</t></is></c>";
    oss << "<c r=\"J12\" t=\"inlineStr\"><is><t>TOTAL</t></is></c></row>\n";

    // Rows 13 to 38: Line items
    for (int idx = 0; idx < 26; ++idx) {
        int r = 13 + idx;
        oss << "<row r=\"" << r << "\">";
        if (idx < invoice.items.size()) {
            const auto& it = invoice.items[idx];
            oss << "<c r=\"C" << r << "\" t=\"inlineStr\"><is><t>" << xmlEscape(it.desc) << "</t></is></c>";
            oss << "<c r=\"E" << r << "\"><v>" << it.unidades << "</v></c>";
            oss << "<c r=\"F" << r << "\"><v>" << QString::number(it.precioUnitario, 'f', 2).toStdString() << "</v></c>";
            if (it.anchoPersianaFinal > 0) {
                oss << "<c r=\"G" << r << "\"><v>" << it.anchoPersianaFinal << "</v></c>";
            }
            if (it.alto > 0) {
                oss << "<c r=\"H" << r << "\"><v>" << it.alto << "</v></c>";
            }
            double m2 = it.calcularMetrosCuadrados();
            if (m2 > 0) {
                oss << "<c r=\"I" << r << "\"><v>" << m2 << "</v></c>";
            }
            double lineTotal = it.calcularTotal();
            oss << "<c r=\"J" << r << "\"><v>" << QString::number(lineTotal, 'f', 2).toStdString() << "</v></c>";
        }
        oss << "</row>\n";
    }

    // Row 39: Base Imponible
    oss << "<row r=\"39\"><c r=\"J39\"><f>SUM(J13:J38)</f><v>" << QString::number(invoice.calcularTotalBruto(), 'f', 2).toStdString() << "</v></c></row>\n";

    // Row 40: Forma de pago
    oss << "<row r=\"40\"><c r=\"B40\" t=\"inlineStr\"><is><t>FORMA DE PAGO……..</t></is></c>";
    oss << "<c r=\"C40\" t=\"inlineStr\"><is><t>" << xmlEscape(invoice.formaPago) << "</t></is></c></row>\n";

    // Row 42: IVA
    int ivaPct = static_cast<int>(invoice.tipoIva * 100.0 + 0.5);
    oss << "<row r=\"42\"><c r=\"F42\" t=\"inlineStr\"><is><t>I.V.A.</t></is></c>";
    oss << "<c r=\"G42\"><v>" << invoice.tipoIva << "</v></c>";
    oss << "<c r=\"J42\"><f>((J39*" << ivaPct << ")/100)</f><v>" << QString::number(invoice.calcularCuotaIva(), 'f', 2).toStdString() << "</v></c></row>\n";

    // Row 43: Total Factura
    oss << "<row r=\"43\"><c r=\"F43\" t=\"inlineStr\"><is><t>TOTAL FACTURA</t></is></c>";
    oss << "<c r=\"J43\"><f>(J39+J42)</f><v>" << QString::number(invoice.calcularTotalFactura(), 'f', 2).toStdString() << "</v></c></row>\n";

    // Rows 45 & 46: Legal Notice
    oss << "<row r=\"45\"><c r=\"B45\" t=\"inlineStr\"><is><t>PROTECCIÓN DE DATOS: Responsable: JUAN MANUEL ALDAO LOPEZ, 52434449S. Finalidad: Gestión de comunicaciones profesionales. Legitimación: Contrato e interés legítimo. Derechos y Más info: Puede ejercer sus derechos en mail persianasagrela@gmail.com.</t></is></c></row>\n";
    oss << "<row r=\"46\"><c r=\"B46\" t=\"inlineStr\"><is><t>CONFIDENCIALIDAD: Este mensaje es privado y dirigido solo al destinatario. La copia o difusión no autorizada está prohibida. Si lo recibe por error, por favor notifíquelo y elimínelo.</t></is></c></row>\n";

    oss << "</sheetData>\n";
    oss << "</worksheet>";
    return oss.str();
}

bool InvoiceGeneratorService::generateExcel(const Invoice& invoice, const QString& outputXlsxPath) {
    QString templatePath = "sample.xlsx";
    if (!QFile::exists(templatePath)) {
        templatePath = QDir(QCoreApplication::applicationDirPath()).filePath("sample.xlsx");
    }

    if (!QFile::exists(templatePath)) {
        qWarning() << "No se encontró la plantilla sample.xlsx en" << templatePath;
        return false;
    }

    mz_zip_archive reader;
    memset(&reader, 0, sizeof(reader));
    if (!mz_zip_reader_init_file(&reader, templatePath.toLocal8Bit().constData(), 0)) {
        qWarning() << "Error abriendo plantilla Excel:" << templatePath;
        return false;
    }

    mz_zip_archive writer;
    memset(&writer, 0, sizeof(writer));
    if (!mz_zip_writer_init_file(&writer, outputXlsxPath.toLocal8Bit().constData(), 0)) {
        qWarning() << "Error creando archivo destino Excel:" << outputXlsxPath;
        mz_zip_reader_end(&reader);
        return false;
    }

    std::string newSheet1 = buildSheet1Xml(invoice);

    int num_files = mz_zip_reader_get_num_files(&reader);
    for (int i = 0; i < num_files; ++i) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&reader, i, &stat)) continue;

        std::string fname = stat.m_filename;
        if (fname == "xl/worksheets/sheet1.xml") {
            mz_zip_writer_add_mem(&writer, fname.c_str(), newSheet1.data(), newSheet1.size(), MZ_DEFAULT_COMPRESSION);
        } else if (fname == "xl/calcChain.xml") {
            continue; // Dejar que Excel recalcule limpiamente
        } else {
            size_t size = 0;
            void* pData = mz_zip_reader_extract_to_heap(&reader, i, &size, 0);
            if (pData) {
                mz_zip_writer_add_mem(&writer, fname.c_str(), pData, size, MZ_DEFAULT_COMPRESSION);
                mz_free(pData);
            }
        }
    }

    mz_zip_reader_end(&reader);
    mz_zip_writer_finalize_archive(&writer);
    mz_zip_writer_end(&writer);

    qDebug() << "Excel generado nativamente en C++:" << outputXlsxPath;
    return QFile::exists(outputXlsxPath);
}

bool InvoiceGeneratorService::generatePdf(const Invoice& invoice, const QString& outputPdfPath) {
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(outputPdfPath);
    printer.setPageSize(QPrinter::A4);
    printer.setPageMargins(12, 12, 12, 12, QPrinter::Millimeter);

    QPainter painter;
    if (!painter.begin(&printer)) {
        qWarning() << "No se pudo iniciar QPainter para el PDF:" << outputPdfPath;
        return false;
    }

    // Coordenadas lógicas normalizadas (800 x 1130 puntos)
    const int W = 800;
    const int H = 1130;
    painter.setWindow(0, 0, W, H);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // Paleta de colores corporativos
    QColor primaryColor("#1F4E78");
    QColor secondaryColor("#2B5B84");
    QColor darkText("#2C3E50");
    QColor mutedText("#595959");
    QColor bgLight("#F2F4F7");
    QColor borderColor("#D0D5DD");

    // Helper para fuentes con tamaño en píxeles normalizados
    auto makeFont = [](int pixelSize, bool bold = false, bool italic = false) {
        QFont f("Arial");
        f.setPixelSize(pixelSize);
        f.setBold(bold);
        f.setItalic(italic);
        return f;
    };

    int y = 10;

    // --- 1. CABECERA ---
    // Logo
    QString logoPath = "logo.jpg";
    if (!QFile::exists(logoPath)) logoPath = "logo.jpeg";
    if (QFile::exists(logoPath)) {
        QPixmap logo(logoPath);
        if (!logo.isNull()) {
            QPixmap scaledLogo = logo.scaled(180, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            painter.drawPixmap(0, y, scaledLogo);
        }
    }

    // Título FACTURA y Metadatos en la derecha
    painter.setFont(makeFont(26, true));
    painter.setPen(primaryColor);
    painter.drawText(QRect(480, y, 320, 36), Qt::AlignRight | Qt::AlignTop, "FACTURA");

    // Caja de Metadatos (Nº Factura, Fecha, Forma de Pago)
    QRect metaBox(480, y + 42, 320, 78);
    painter.setBrush(bgLight);
    painter.setPen(borderColor);
    painter.drawRoundedRect(metaBox, 6, 6);

    painter.setPen(darkText);
    painter.setFont(makeFont(11, true));
    painter.drawText(metaBox.adjusted(12, 10, -12, 0), Qt::AlignLeft, "Número:");
    painter.drawText(metaBox.adjusted(12, 32, -12, 0), Qt::AlignLeft, "Fecha:");
    painter.drawText(metaBox.adjusted(12, 54, -12, 0), Qt::AlignLeft, "Forma de Pago:");

    painter.setFont(makeFont(11, false));
    painter.drawText(metaBox.adjusted(0, 10, -12, 0), Qt::AlignRight, QString("Nº %1").arg(invoice.numeroFactura));
    painter.drawText(metaBox.adjusted(0, 32, -12, 0), Qt::AlignRight, invoice.fecha.toString("dd/MM/yyyy"));
    painter.drawText(metaBox.adjusted(0, 54, -12, 0), Qt::AlignRight, invoice.formaPago);

    // Datos Emisor (debajo del logo)
    int ey = y + 88;
    painter.setFont(makeFont(11, true));
    painter.setPen(primaryColor);
    painter.drawText(0, ey, invoice.emisorNombre);
    ey += 16;
    painter.setFont(makeFont(10, false));
    painter.setPen(mutedText);
    painter.drawText(0, ey, QString("%1 - %2").arg(invoice.emisorDireccion, invoice.emisorPoligono));
    ey += 15;
    painter.drawText(0, ey, QString("%1 %2 | %3").arg(invoice.emisorCp, invoice.emisorCiudad, invoice.emisorCif));

    // --- 2. BLOQUE DATOS DEL CLIENTE ---
    y = 155;
    QRect clientBox(0, y, W, 88);
    painter.setBrush(bgLight);
    painter.setPen(borderColor);
    painter.drawRoundedRect(clientBox, 6, 6);

    // Encabezado del bloque cliente
    painter.setBrush(secondaryColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRect(0, y, W, 24), 6, 6);
    painter.setPen(Qt::white);
    painter.setFont(makeFont(11, true));
    painter.drawText(QRect(12, y, W - 24, 24), Qt::AlignVCenter | Qt::AlignLeft, "DATOS DEL CLIENTE");

    // Contenido cliente
    painter.setPen(darkText);
    int cy = y + 36;
    painter.setFont(makeFont(10, true));
    painter.drawText(12, cy, "Nombre / Razón Social:");
    painter.setFont(makeFont(10, false));
    painter.drawText(160, cy, invoice.cliente.nombre.isEmpty() ? "-" : invoice.cliente.nombre);

    painter.setFont(makeFont(10, true));
    painter.drawText(500, cy, "CIF/NIF:");
    painter.setFont(makeFont(10, false));
    painter.drawText(570, cy, invoice.cliente.cifNif.isEmpty() ? "-" : invoice.cliente.cifNif);

    cy += 18;
    painter.setFont(makeFont(10, true));
    painter.drawText(12, cy, "Dirección:");
    painter.setFont(makeFont(10, false));
    painter.drawText(160, cy, invoice.cliente.direccion.isEmpty() ? "-" : invoice.cliente.direccion);

    cy += 18;
    painter.setFont(makeFont(10, true));
    painter.drawText(12, cy, "Población / Provincia:");
    painter.setFont(makeFont(10, false));
    QString pobProv = invoice.cliente.poblacion;
    if (!invoice.cliente.provincia.isEmpty()) pobProv += " (" + invoice.cliente.provincia + ")";
    painter.drawText(160, cy, pobProv.isEmpty() ? "-" : pobProv);

    // --- 3. TABLA DE ARTÍCULOS ---
    y = 255;
    int headerHeight = 28;

    painter.setBrush(primaryColor);
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, y, W, headerHeight);

    // Columnas proporcionales (Total W = 800)
    int colDescW = 320; // 0..320
    int colUdsW  = 65;  // 320..385
    int colPrecW = 95;  // 385..480
    int colAnchW = 85;  // 480..565
    int colAltoW = 75;  // 565..640
    int colM2W   = 70;  // 640..710
    int colTotW  = 90;  // 710..800

    int x0 = 0;
    int x1 = x0 + colDescW;
    int x2 = x1 + colUdsW;
    int x3 = x2 + colPrecW;
    int x4 = x3 + colAnchW;
    int x5 = x4 + colAltoW;
    int x6 = x5 + colM2W;

    painter.setPen(Qt::white);
    painter.setFont(makeFont(10, true));
    painter.drawText(QRect(x0 + 10, y, colDescW - 14, headerHeight), Qt::AlignVCenter | Qt::AlignLeft, "DESCRIPCIÓN");
    painter.drawText(QRect(x1, y, colUdsW, headerHeight), Qt::AlignCenter, "UDS.");
    painter.drawText(QRect(x2, y, colPrecW - 8, headerHeight), Qt::AlignVCenter | Qt::AlignRight, "PRECIO");
    painter.drawText(QRect(x3, y, colAnchW, headerHeight), Qt::AlignCenter, "ANCHO (mm)");
    painter.drawText(QRect(x4, y, colAltoW, headerHeight), Qt::AlignCenter, "ALTO (mm)");
    painter.drawText(QRect(x5, y, colM2W, headerHeight), Qt::AlignCenter, "M²");
    painter.drawText(QRect(x6, y, colTotW - 10, headerHeight), Qt::AlignVCenter | Qt::AlignRight, "TOTAL");

    y += headerHeight;
    int rowHeight = 26;

    for (int i = 0; i < invoice.items.size(); ++i) {
        const auto& it = invoice.items[i];
        
        // Cebra de filas
        painter.setBrush(i % 2 == 1 ? bgLight : Qt::white);
        painter.setPen(borderColor);
        painter.drawRect(0, y, W, rowHeight);

        painter.setPen(darkText);
        painter.setFont(makeFont(10, false));

        // Descripción (con truncado si excede ancho)
        QString descText = it.desc;
        QFontMetrics fm(painter.font());
        QString elidedDesc = fm.elidedText(descText, Qt::ElideRight, colDescW - 18);
        painter.drawText(QRect(x0 + 10, y, colDescW - 14, rowHeight), Qt::AlignVCenter | Qt::AlignLeft, elidedDesc);
        
        // Uds
        painter.drawText(QRect(x1, y, colUdsW, rowHeight), Qt::AlignCenter, QString::number(it.unidades, 'f', 0));
        
        // Precio
        painter.drawText(QRect(x2, y, colPrecW - 8, rowHeight), Qt::AlignVCenter | Qt::AlignRight, QString("%1 €").arg(it.precioUnitario, 0, 'f', 2));
        
        // ANCHO: Presenta el ancho de la persiana final
        QString anchoTxt = (it.anchoPersianaFinal > 0) ? QString::number(it.anchoPersianaFinal, 'f', 0) : "-";
        painter.drawText(QRect(x3, y, colAnchW, rowHeight), Qt::AlignCenter, anchoTxt);

        // ALTO
        QString altoTxt = (it.alto > 0) ? QString::number(it.alto, 'f', 0) : "-";
        painter.drawText(QRect(x4, y, colAltoW, rowHeight), Qt::AlignCenter, altoTxt);

        // M²: Calculado con el ancho del rollo
        double m2 = it.calcularMetrosCuadrados();
        QString m2Txt = (m2 > 0) ? QString::number(m2, 'f', 3) : "-";
        painter.drawText(QRect(x5, y, colM2W, rowHeight), Qt::AlignCenter, m2Txt);

        // Total Línea
        painter.setFont(makeFont(10, true));
        painter.drawText(QRect(x6, y, colTotW - 10, rowHeight), Qt::AlignVCenter | Qt::AlignRight, QString("%1 €").arg(it.calcularTotal(), 0, 'f', 2));

        y += rowHeight;
    }

    // --- 4. BLOQUE DE TOTALES ---
    y += 24;
    QRect totalsBox(460, y, 340, 115);
    painter.setBrush(bgLight);
    painter.setPen(borderColor);
    painter.drawRoundedRect(totalsBox, 6, 6);

    painter.setPen(darkText);
    painter.setFont(makeFont(11, false));
    painter.drawText(totalsBox.adjusted(15, 14, -15, 0), Qt::AlignLeft, "Total Bruto (Base Imp.):");
    painter.drawText(totalsBox.adjusted(0, 14, -15, 0), Qt::AlignRight, QString("%1 €").arg(invoice.calcularTotalBruto(), 0, 'f', 2));

    painter.drawText(totalsBox.adjusted(15, 42, -15, 0), Qt::AlignLeft, QString("I.V.A. (%1%):").arg(int(invoice.tipoIva * 100)));
    painter.drawText(totalsBox.adjusted(0, 42, -15, 0), Qt::AlignRight, QString("%1 €").arg(invoice.calcularCuotaIva(), 0, 'f', 2));

    // Separador
    painter.setPen(borderColor);
    painter.drawLine(totalsBox.left() + 10, totalsBox.top() + 72, totalsBox.right() - 10, totalsBox.top() + 72);

    // Total Factura destacado
    painter.setFont(makeFont(14, true));
    painter.setPen(primaryColor);
    painter.drawText(totalsBox.adjusted(15, 82, -15, 0), Qt::AlignLeft, "TOTAL FACTURA:");
    painter.drawText(totalsBox.adjusted(0, 82, -15, 0), Qt::AlignRight, QString("%1 €").arg(invoice.calcularTotalFactura(), 0, 'f', 2));

    // --- 5. PIE DE PÁGINA (PROTECCIÓN DE DATOS Y CONFIDENCIALIDAD) ---
    int footerY = H - 60;
    painter.setPen(QColor("#CBD5E1"));
    painter.drawLine(0, footerY - 8, W, footerY - 8);

    painter.setFont(makeFont(8, false, false));
    painter.setPen(QColor("#64748B"));

    QString textProteccion = "PROTECCIÓN DE DATOS: Responsable: JUAN MANUEL ALDAO LOPEZ, 52434449S. Finalidad: Gestión de comunicaciones profesionales. Legitimación: Contrato e interés legítimo. Derechos y Más info: Puede ejercer sus derechos en mail persianasagrela@gmail.com.";
    QString textConfidencialidad = "CONFIDENCIALIDAD: Este mensaje es privado y dirigido solo al destinatario. La copia o difusión no autorizada está prohibida. Si lo recibe por error, por favor notifíquelo y elimínelo.";

    painter.drawText(QRect(0, footerY, W, 24), Qt::AlignCenter | Qt::TextWordWrap, textProteccion);
    painter.drawText(QRect(0, footerY + 25, W, 24), Qt::AlignCenter | Qt::TextWordWrap, textConfidencialidad);

    painter.end();
    qDebug() << "PDF generado con éxito:" << outputPdfPath;
    return true;
}
