#include "CatalogService.h"
#include "../utils/ExcelReader.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QProcess>
#include <QRegularExpression>
#include <QDateTime>
#include <QDebug>

CatalogService& CatalogService::instance() {
    static CatalogService inst;
    return inst;
}

// Función para normalizar texto eliminando acentos, diacríticos y pasando a minúsculas
static QString removeAccents(const QString& str) {
    QString normalized = str.normalized(QString::NormalizationForm_D);
    QString result;
    result.reserve(normalized.size());
    for (const QChar& ch : normalized) {
        if (ch.category() != QChar::Mark_NonSpacing) {
            result.append(ch);
        }
    }
    return result.toLower();
}

bool CatalogService::loadFromExcel(const QString& xlsxPath) {
    if (!QFile::exists(xlsxPath)) {
        return false;
    }

    auto rows = ExcelReader::readXlsx(xlsxPath);
    if (rows.size() <= 1) {
        return false;
    }

    QVector<CatalogItem> items;
    for (int i = 1; i < rows.size(); ++i) {
        const auto& r = rows[i];
        if (r.size() < 3) continue;

        CatalogItem it;
        it.code = r[0].trimmed();
        it.desc = r[1].trimmed();
        if (it.desc.isEmpty() && it.code.isEmpty()) continue;

        it.pvp = r[2].trimmed().toDouble();
        it.p1 = it.pvp;
        it.t1 = (r.size() > 3 && !r[3].trimmed().isEmpty()) ? r[3].trimmed().toDouble() : it.pvp;
        it.p_t1 = it.t1;
        it.u1 = (r.size() > 4 && !r[4].trimmed().isEmpty()) ? r[4].trimmed() : "ud.";
        it.sheet = (r.size() > 5 && !r[5].trimmed().isEmpty()) ? r[5].trimmed() : "General";
        it.imgPath = (r.size() > 6) ? r[6].trimmed() : "";

        items.append(it);
    }

    if (!items.isEmpty()) {
        m_items = items;
        qDebug() << "Catálogo cargado directamente desde Excel (" << xlsxPath << "):" << m_items.size() << "artículos.";

        // Sincronizar copia a JSON
        QJsonArray arr;
        for (const auto& it : m_items) {
            arr.append(it.toJson());
        }
        QFile f("catalog.json");
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(QJsonDocument(arr).toJson());
            f.close();
        }
        return true;
    }
    return false;
}

bool CatalogService::syncWithPreciosFolder(const QString& folderPath, const QString& jsonPath, bool force) {
    // 1. Si existe precios_catalogo.xlsx y es más nuevo o force, cargar desde Excel
    QString excelPath = "precios_catalogo.xlsx";
    if (QFile::exists(excelPath)) {
        QFileInfo xlInfo(excelPath);
        QFileInfo jsonInfo(jsonPath);
        if (force || !jsonInfo.exists() || xlInfo.lastModified() > jsonInfo.lastModified()) {
            if (loadFromExcel(excelPath)) {
                return true;
            }
        }
    }

    bool needsUpdate = force;

    if (!QFile::exists(jsonPath)) {
        needsUpdate = true;
    } else if (QDir(folderPath).exists()) {
        QFileInfo jsonInfo(jsonPath);
        QDateTime jsonModTime = jsonInfo.lastModified();

        QDir dir(folderPath);
        QFileInfoList files = dir.entryInfoList({"*.xls", "*.xlsx", "*.pdf"}, QDir::Files);
        for (const auto& fi : files) {
            if (fi.lastModified() > jsonModTime) {
                needsUpdate = true;
                qDebug() << "Detectado archivo modificado en PRECIOS:" << fi.fileName();
                break;
            }
        }
    }

    if (needsUpdate) {
        runExtractionScript(folderPath, jsonPath);
    }

    return loadCatalog(jsonPath);
}

void CatalogService::runExtractionScript(const QString& folderPath, const QString& jsonPath) {
    QString scriptPath = "extract_catalog_precios.py";
    if (QFile::exists(scriptPath)) {
        QString pythonBin = "/home/enrique/anaconda3/bin/python";
        if (!QFile::exists(pythonBin)) pythonBin = "python3";

        qDebug() << "Extrayendo tarifas actualizadas de PRECIOS...";
        QProcess process;
        process.start(pythonBin, QStringList() << scriptPath << folderPath << jsonPath);
        process.waitForFinished(15000);
        qDebug() << "Tarifas de PRECIOS extraídas correctamente.";
    }
}

bool CatalogService::loadCatalog(const QString& jsonPath) {
    // Si existe precios_catalogo.xlsx y es más reciente que el json, cargar de Excel
    QString excelPath = "precios_catalogo.xlsx";
    if (QFile::exists(excelPath)) {
        QFileInfo xlInfo(excelPath);
        QFileInfo jsonInfo(jsonPath);
        if (!jsonInfo.exists() || xlInfo.lastModified() > jsonInfo.lastModified()) {
            if (loadFromExcel(excelPath)) {
                return true;
            }
        }
    }

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (QFile::exists(excelPath)) {
            return loadFromExcel(excelPath);
        }
        qWarning() << "No se pudo abrir el archivo de catálogo:" << jsonPath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "El formato del catálogo JSON no es un array válido.";
        return false;
    }

    m_items.clear();
    QJsonArray arr = doc.array();
    for (const auto& val : arr) {
        if (val.isObject()) {
            m_items.append(CatalogItem::fromJson(val.toObject()));
        }
    }

    qDebug() << "Catálogo cargado con éxito:" << m_items.size() << "artículos en PVP.";
    return true;
}

bool CatalogService::addCustomItem(const CatalogItem& item, const QString& jsonPath, const QString& xlsxPath) {
    bool replaced = false;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].code.compare(item.code, Qt::CaseInsensitive) == 0) {
            m_items[i] = item;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        m_items.append(item);
    }

    return saveCatalog(jsonPath, xlsxPath);
}

bool CatalogService::saveCatalog(const QString& jsonPath, const QString& xlsxPath) {
    QJsonArray arr;
    for (const auto& it : m_items) {
        arr.append(it.toJson());
    }
    QJsonDocument doc(arr);
    QFile fJson(jsonPath);
    if (fJson.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fJson.write(doc.toJson());
        fJson.close();
    }

    if (QDir("windows").exists()) {
        QFile fWinJson("windows/catalog.json");
        if (fWinJson.open(QIODevice::WriteOnly | QIODevice::Text)) {
            fWinJson.write(doc.toJson());
            fWinJson.close();
        }
    }

    return true;
}

const QVector<CatalogItem>& CatalogService::getAllItems() const {
    return m_items;
}

QStringList CatalogService::getSheets() const {
    QSet<QString> sheets;
    for (const auto& it : m_items) {
        if (!it.sheet.isEmpty()) {
            sheets.insert(it.sheet);
        }
    }
    QStringList sorted = sheets.values();
    sorted.sort();
    return sorted;
}

QStringList CatalogService::getCategoriesForSheet(const QString& sheet) const {
    QSet<QString> cats;
    for (const auto& it : m_items) {
        if (sheet == "Todas" || it.sheet.compare(sheet, Qt::CaseInsensitive) == 0) {
            if (!it.category.isEmpty()) {
                cats.insert(it.category);
            }
        }
    }
    QStringList sorted = cats.values();
    sorted.sort();
    return sorted;
}

QVector<CatalogItem> CatalogService::search(const QString& text, const QString& sheetFilter) const {
    if (text.trimmed().isEmpty() && (sheetFilter == "Todas" || sheetFilter.isEmpty())) {
        return m_items;
    }

    QString normText = removeAccents(text.trimmed());
    QStringList tokens = normText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    QVector<CatalogItem> results;
    for (const auto& it : m_items) {
        if (sheetFilter != "Todas" && !sheetFilter.isEmpty()) {
            if (it.sheet.compare(sheetFilter, Qt::CaseInsensitive) != 0) {
                continue;
            }
        }

        if (tokens.isEmpty()) {
            results.append(it);
            continue;
        }

        QString itemDescNorm = removeAccents(it.desc);
        QString itemCodeNorm = removeAccents(it.code);

        bool allMatch = true;
        for (const auto& tok : tokens) {
            if (!itemDescNorm.contains(tok) && !itemCodeNorm.contains(tok)) {
                allMatch = false;
                break;
            }
        }

        if (allMatch) {
            results.append(it);
        }
    }

    return results;
}

CatalogItem CatalogService::findByCode(const QString& code) const {
    for (const auto& it : m_items) {
        if (it.code.compare(code, Qt::CaseInsensitive) == 0) {
            return it;
        }
    }
    return CatalogItem();
}
