#include "TransactionService.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include <sstream>
#include <iomanip>
#include <cmath>

extern "C" {
#include "miniz.h"
}

TransactionService& TransactionService::instance() {
    static TransactionService inst;
    return inst;
}

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

bool TransactionService::loadTransactions(const QString& salesJson, const QString& purchasesJson) {
    m_salesPath = salesJson;
    m_purchasesPath = purchasesJson;

    m_sales.clear();
    m_purchases.clear();

    // 1. Cargar Ventas
    QFile fSales(salesJson);
    if (fSales.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = fSales.readAll();
        fSales.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            for (const auto& val : doc.array()) {
                if (val.isObject()) {
                    m_sales.append(SaleRecord::fromJson(val.toObject()));
                }
            }
        }
    }

    // 2. Cargar Compras
    QFile fPurchases(purchasesJson);
    if (fPurchases.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = fPurchases.readAll();
        fPurchases.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            for (const auto& val : doc.array()) {
                if (val.isObject()) {
                    m_purchases.append(PurchaseRecord::fromJson(val.toObject()));
                }
            }
        }
    }

    qDebug() << "TransactionService cargado:" << m_sales.size() << "ventas y" << m_purchases.size() << "compras.";
    return true;
}

bool TransactionService::saveSales(const QString& salesJson) {
    QJsonArray arr;
    for (const auto& s : m_sales) {
        arr.append(s.toJson());
    }
    QFile file(salesJson);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    file.write(QJsonDocument(arr).toJson());
    file.close();
    return true;
}

bool TransactionService::savePurchases(const QString& purchasesJson) {
    QJsonArray arr;
    for (const auto& p : m_purchases) {
        arr.append(p.toJson());
    }
    QFile file(purchasesJson);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    file.write(QJsonDocument(arr).toJson());
    file.close();
    return true;
}

void TransactionService::recordSale(const Invoice& invoice, const QString& excelPath, const QString& pdfPath) {
    SaleRecord newSale = SaleRecord::fromInvoice(invoice, excelPath, pdfPath);
    
    // Si ya existe una venta con el mismo Nº de factura, la actualizamos
    bool updated = false;
    for (int i = 0; i < m_sales.size(); ++i) {
        if (m_sales[i].numeroFactura == newSale.numeroFactura && !newSale.numeroFactura.isEmpty()) {
            m_sales[i] = newSale;
            updated = true;
            break;
        }
    }

    if (!updated) {
        m_sales.append(newSale);
    }

    saveSales(m_salesPath);
    exportToExcel("registro_compras_ventas.xlsx");
}

void TransactionService::addPurchase(const PurchaseRecord& purchase) {
    PurchaseRecord rec = purchase;
    if (rec.id.isEmpty()) {
        rec.id = QString("PUR_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(m_purchases.size() + 1);
    }
    m_purchases.append(rec);
    savePurchases(m_purchasesPath);
    exportToExcel("registro_compras_ventas.xlsx");
}

bool TransactionService::removeSale(const QString& id) {
    for (int i = 0; i < m_sales.size(); ++i) {
        if (m_sales[i].id == id) {
            m_sales.removeAt(i);
            saveSales(m_salesPath);
            exportToExcel("registro_compras_ventas.xlsx");
            return true;
        }
    }
    return false;
}

bool TransactionService::removePurchase(const QString& id) {
    for (int i = 0; i < m_purchases.size(); ++i) {
        if (m_purchases[i].id == id) {
            m_purchases.removeAt(i);
            savePurchases(m_purchasesPath);
            exportToExcel("registro_compras_ventas.xlsx");
            return true;
        }
    }
    return false;
}

const QVector<SaleRecord>& TransactionService::getSales() const {
    return m_sales;
}

const QVector<PurchaseRecord>& TransactionService::getPurchases() const {
    return m_purchases;
}

double TransactionService::getTotalVentasBruto() const {
    double total = 0.0;
    for (const auto& s : m_sales) total += s.baseImponible;
    return std::round(total * 100.0) / 100.0;
}

double TransactionService::getTotalVentasIva() const {
    double total = 0.0;
    for (const auto& s : m_sales) total += s.cuotaIva;
    return std::round(total * 100.0) / 100.0;
}

double TransactionService::getTotalVentasFacturado() const {
    double total = 0.0;
    for (const auto& s : m_sales) total += s.totalFactura;
    return std::round(total * 100.0) / 100.0;
}

double TransactionService::getTotalComprasBase() const {
    double total = 0.0;
    for (const auto& p : m_purchases) total += p.calcularBase();
    return std::round(total * 100.0) / 100.0;
}

double TransactionService::getTotalComprasIva() const {
    double total = 0.0;
    for (const auto& p : m_purchases) total += p.calcularIva();
    return std::round(total * 100.0) / 100.0;
}

double TransactionService::getTotalComprasFacturado() const {
    double total = 0.0;
    for (const auto& p : m_purchases) total += p.calcularTotal();
    return std::round(total * 100.0) / 100.0;
}

bool TransactionService::exportToExcel(const QString& outputPath) {
    mz_zip_archive writer;
    memset(&writer, 0, sizeof(writer));
    if (!mz_zip_writer_init_file(&writer, outputPath.toLocal8Bit().constData(), 0)) {
        qWarning() << "Error creando libro de registro Excel:" << outputPath;
        return false;
    }

    // 1. [Content_Types].xml
    std::string contentTypes =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
        "  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
        "  <Default Extension=\"xml\" ContentType=\"application/xml\"/>\n"
        "  <Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>\n"
        "  <Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>\n"
        "  <Override PartName=\"/xl/worksheets/sheet2.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>\n"
        "</Types>";

    // 2. _rels/.rels
    std::string dotRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "  <Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>\n"
        "</Relationships>";

    // 3. xl/workbook.xml
    std::string workbookXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">\n"
        "  <sheets>\n"
        "    <sheet name=\"Ventas\" sheetId=\"1\" r:id=\"rId1\"/>\n"
        "    <sheet name=\"Compras\" sheetId=\"2\" r:id=\"rId2\"/>\n"
        "  </sheets>\n"
        "</workbook>";

    // 4. xl/_rels/workbook.xml.rels
    std::string workbookRels =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
        "  <Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>\n"
        "  <Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet2.xml\"/>\n"
        "</Relationships>";

    // 5. xl/worksheets/sheet1.xml (Ventas)
    std::ostringstream s1;
    s1 << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    s1 << "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n";
    s1 << "<sheetData>\n";
    s1 << "<row r=\"1\">";
    s1 << "<c r=\"A1\" t=\"inlineStr\"><is><t>Fecha</t></is></c>";
    s1 << "<c r=\"B1\" t=\"inlineStr\"><is><t>Nº Factura</t></is></c>";
    s1 << "<c r=\"C1\" t=\"inlineStr\"><is><t>Cliente</t></is></c>";
    s1 << "<c r=\"D1\" t=\"inlineStr\"><is><t>CIF / NIF</t></is></c>";
    s1 << "<c r=\"E1\" t=\"inlineStr\"><is><t>Base Imponible (€)</t></is></c>";
    s1 << "<c r=\"F1\" t=\"inlineStr\"><is><t>Cuota IVA (€)</t></is></c>";
    s1 << "<c r=\"G1\" t=\"inlineStr\"><is><t>Total Factura (€)</t></is></c>";
    s1 << "<c r=\"H1\" t=\"inlineStr\"><is><t>Nº Artículos</t></is></c>";
    s1 << "</row>\n";

    for (int i = 0; i < m_sales.size(); ++i) {
        int r = i + 2;
        const auto& sale = m_sales[i];
        s1 << "<row r=\"" << r << "\">";
        s1 << "<c r=\"A" << r << "\" t=\"inlineStr\"><is><t>" << xmlEscape(sale.fecha.toString("dd/MM/yyyy")) << "</t></is></c>";
        s1 << "<c r=\"B" << r << "\" t=\"inlineStr\"><is><t>" << xmlEscape(sale.numeroFactura) << "</t></is></c>";
        s1 << "<c r=\"C" << r << "\" t=\"inlineStr\"><is><t>" << xmlEscape(sale.cliente.nombre) << "</t></is></c>";
        s1 << "<c r=\"D" << r << "\" t=\"inlineStr\"><is><t>" << xmlEscape(sale.cliente.cifNif) << "</t></is></c>";
        s1 << "<c r=\"E" << r << "\"><v>" << QString::number(sale.baseImponible, 'f', 2).toStdString() << "</v></c>";
        s1 << "<c r=\"F" << r << "\"><v>" << QString::number(sale.cuotaIva, 'f', 2).toStdString() << "</v></c>";
        s1 << "<c r=\"G" << r << "\"><v>" << QString::number(sale.totalFactura, 'f', 2).toStdString() << "</v></c>";
        s1 << "<c r=\"H" << r << "\"><v>" << sale.numArticulos << "</v></c>";
        s1 << "</row>\n";
    }

    int totSaleRow = m_sales.size() + 2;
    s1 << "<row r=\"" << totSaleRow << "\">";
    s1 << "<c r=\"A" << totSaleRow << "\" t=\"inlineStr\"><is><t>TOTALES VENTAS</t></is></c>";
    s1 << "<c r=\"E" << totSaleRow << "\"><f>SUM(E2:E" << (totSaleRow - 1) << ")</f><v>" << QString::number(getTotalVentasBruto(), 'f', 2).toStdString() << "</v></c>";
    s1 << "<c r=\"F" << totSaleRow << "\"><f>SUM(F2:F" << (totSaleRow - 1) << ")</f><v>" << QString::number(getTotalVentasIva(), 'f', 2).toStdString() << "</v></c>";
    s1 << "<c r=\"G" << totSaleRow << "\"><f>SUM(G2:G" << (totSaleRow - 1) << ")</f><v>" << QString::number(getTotalVentasFacturado(), 'f', 2).toStdString() << "</v></c>";
    s1 << "</row>\n";

    s1 << "</sheetData>\n</worksheet>";
    std::string sheet1Xml = s1.str();

    // 6. xl/worksheets/sheet2.xml (Compras)
    std::ostringstream s2;
    s2 << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
    s2 << "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n";
    s2 << "<sheetData>\n";
    s2 << "<row r=\"1\">";
    s2 << "<c r=\"A1\" t=\"inlineStr\"><is><t>Fecha</t></is></c>";
    s2 << "<c r=\"B1\" t=\"inlineStr\"><is><t>Proveedor</t></is></c>";
    s2 << "<c r=\"C1\" t=\"inlineStr\"><is><t>CIF / NIF</t></is></c>";
    s2 << "<c r=\"D1\" t=\"inlineStr\"><is><t>Nº Factura Prov.</t></is></c>";
    s2 << "<c r=\"E1\" t=\"inlineStr\"><is><t>Concepto / Artículo</t></is></c>";
    s2 << "<c r=\"F1\" t=\"inlineStr\"><is><t>Unidades</t></is></c>";
    s2 << "<c r=\"G1\" t=\"inlineStr\"><is><t>Precio Unitario (€)</t></is></c>";
    s2 << "<c r=\"H1\" t=\"inlineStr\"><is><t>Base Imponible (€)</t></is></c>";
    s2 << "<c r=\"I1\" t=\"inlineStr\"><is><t>Cuota IVA (€)</t></is></c>";
    s2 << "<c r=\"J1\" t=\"inlineStr\"><is><t>Total Compra (€)</t></is></c>";
    s2 << "</row>\n";

    for (int i = 0; i < m_purchases.size(); ++i) {
        int r = i + 2;
        const auto& pur = m_purchases[i];
        s2 << "<row r=\"" << r << "\">";
        s2 << "<c r=\"A" << r << "\" t=\"inlineStr\"><is><t>" << xmlEscape(pur.fecha.toString("dd/MM/yyyy")) << "</t></is></c>";
        s2 << "<c r=\"B" << r << "\" t=\"inlineStr\"><is><t>" << xmlEscape(pur.proveedor) << "</t></is></c>";
        s2 << "<c r=\"C" << r << "\" t=\"inlineStr\"><is><t>" << xmlEscape(pur.cifNif) << "</t></is></c>";
        s2 << "<c r=\"D" << r << "\" t=\"inlineStr\"><is><t>" << xmlEscape(pur.numFacturaProveedor) << "</t></is></c>";
        s2 << "<c r=\"E" << r << "\" t=\"inlineStr\"><is><t>" << xmlEscape(pur.concepto) << "</t></is></c>";
        s2 << "<c r=\"F" << r << "\"><v>" << pur.unidades << "</v></c>";
        s2 << "<c r=\"G" << r << "\"><v>" << QString::number(pur.precioUnitario, 'f', 2).toStdString() << "</v></c>";
        s2 << "<c r=\"H" << r << "\"><v>" << QString::number(pur.calcularBase(), 'f', 2).toStdString() << "</v></c>";
        s2 << "<c r=\"I" << r << "\"><v>" << QString::number(pur.calcularIva(), 'f', 2).toStdString() << "</v></c>";
        s2 << "<c r=\"J" << r << "\"><v>" << QString::number(pur.calcularTotal(), 'f', 2).toStdString() << "</v></c>";
        s2 << "</row>\n";
    }

    int totPurRow = m_purchases.size() + 2;
    s2 << "<row r=\"" << totPurRow << "\">";
    s2 << "<c r=\"A" << totPurRow << "\" t=\"inlineStr\"><is><t>TOTALES COMPRAS</t></is></c>";
    s2 << "<c r=\"H" << totPurRow << "\"><f>SUM(H2:H" << (totPurRow - 1) << ")</f><v>" << QString::number(getTotalComprasBase(), 'f', 2).toStdString() << "</v></c>";
    s2 << "<c r=\"I" << totPurRow << "\"><f>SUM(I2:I" << (totPurRow - 1) << ")</f><v>" << QString::number(getTotalComprasIva(), 'f', 2).toStdString() << "</v></c>";
    s2 << "<c r=\"J" << totPurRow << "\"><f>SUM(J2:J" << (totPurRow - 1) << ")</f><v>" << QString::number(getTotalComprasFacturado(), 'f', 2).toStdString() << "</v></c>";
    s2 << "</row>\n";

    s2 << "</sheetData>\n</worksheet>";
    std::string sheet2Xml = s2.str();

    // Añadir todos los componentes al archivo ZIP Excel
    mz_zip_writer_add_mem(&writer, "[Content_Types].xml", contentTypes.data(), contentTypes.size(), MZ_DEFAULT_COMPRESSION);
    mz_zip_writer_add_mem(&writer, "_rels/.rels", dotRels.data(), dotRels.size(), MZ_DEFAULT_COMPRESSION);
    mz_zip_writer_add_mem(&writer, "xl/workbook.xml", workbookXml.data(), workbookXml.size(), MZ_DEFAULT_COMPRESSION);
    mz_zip_writer_add_mem(&writer, "xl/_rels/workbook.xml.rels", workbookRels.data(), workbookRels.size(), MZ_DEFAULT_COMPRESSION);
    mz_zip_writer_add_mem(&writer, "xl/worksheets/sheet1.xml", sheet1Xml.data(), sheet1Xml.size(), MZ_DEFAULT_COMPRESSION);
    mz_zip_writer_add_mem(&writer, "xl/worksheets/sheet2.xml", sheet2Xml.data(), sheet2Xml.size(), MZ_DEFAULT_COMPRESSION);

    mz_zip_writer_finalize_archive(&writer);
    mz_zip_writer_end(&writer);

    qDebug() << "Registro de Compras y Ventas exportado a Excel:" << outputPath;
    return QFile::exists(outputPath);
}
