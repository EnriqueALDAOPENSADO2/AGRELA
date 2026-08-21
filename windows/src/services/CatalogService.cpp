#include "CatalogService.h"
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

bool CatalogService::syncWithPreciosFolder(const QString& folderPath, const QString& jsonPath, bool force) {
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
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
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

const QVector<CatalogItem>& CatalogService::getAllItems() const {
    return m_items;
}

QStringList CatalogService::getSheets() const {
    QStringList sheets;
    sheets << "Todas";
    QSet<QString> seen;
    for (const auto& item : m_items) {
        if (!item.sheet.isEmpty() && !seen.contains(item.sheet)) {
            seen.insert(item.sheet);
            sheets << item.sheet;
        }
    }
    return sheets;
}

QStringList CatalogService::getCategoriesForSheet(const QString& sheet) const {
    QStringList categories;
    QSet<QString> seen;
    for (const auto& item : m_items) {
        if (sheet == "Todas" || item.sheet == sheet) {
            if (!item.category.isEmpty() && !seen.contains(item.category)) {
                seen.insert(item.category);
                categories << item.category;
            }
        }
    }
    return categories;
}

QVector<CatalogItem> CatalogService::search(const QString& text, const QString& sheetFilter) const {
    QVector<CatalogItem> results;
    QString cleanQuery = removeAccents(text).trimmed();
    QStringList terms = cleanQuery.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    for (const auto& item : m_items) {
        if (sheetFilter != "Todas" && item.sheet != sheetFilter) {
            continue;
        }

        if (terms.isEmpty()) {
            results.append(item);
            continue;
        }

        // Crear una cadena compuesta y normalizada con código, descripción, categoría y hoja
        QString itemText = removeAccents(item.code + " " + item.desc + " " + item.category + " " + item.sheet);
        
        bool allMatch = true;
        for (const QString& term : terms) {
            if (!itemText.contains(term)) {
                allMatch = false;
                break;
            }
        }

        if (allMatch) {
            results.append(item);
        }
    }
    return results;
}

CatalogItem CatalogService::findByCode(const QString& code) const {
    QString cleanTarget = code.trimmed();
    for (const auto& item : m_items) {
        if (item.code.trimmed() == cleanTarget) {
            return item;
        }
    }
    return CatalogItem();
}
