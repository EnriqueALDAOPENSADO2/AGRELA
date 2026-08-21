#include "ClientService.h"
#include "../utils/ExcelReader.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

ClientService& ClientService::instance() {
    static ClientService inst;
    return inst;
}

// Normalización Unicode para eliminar acentos, diacríticos y pasar a minúsculas
static QString stripAccents(const QString& str) {
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

bool ClientService::loadFromExcel(const QString& xlsxPath) {
    if (!QFile::exists(xlsxPath)) {
        return false;
    }

    auto rows = ExcelReader::readXlsx(xlsxPath);
    if (rows.size() <= 1) {
        return false;
    }

    QVector<Customer> clients;
    for (int i = 1; i < rows.size(); ++i) {
        const auto& r = rows[i];
        if (r.size() < 2) continue;

        Customer c;
        c.alias = r[0].trimmed();
        c.nombre = (r.size() > 1) ? r[1].trimmed() : "";
        if (c.alias.isEmpty() && c.nombre.isEmpty()) continue;

        c.cifNif = (r.size() > 2) ? r[2].trimmed() : "";
        c.direccion = (r.size() > 3) ? r[3].trimmed() : "";
        c.poblacion = (r.size() > 4) ? r[4].trimmed() : "";
        c.provincia = (r.size() > 5) ? r[5].trimmed() : "";
        c.telefono = (r.size() > 6) ? r[6].trimmed() : "";
        c.email = (r.size() > 7) ? r[7].trimmed() : "";

        clients.append(c);
    }

    if (!clients.isEmpty()) {
        m_clients = clients;
        qDebug() << "Clientes cargados directamente desde Excel (" << xlsxPath << "):" << m_clients.size() << "clientes.";

        // Sincronizar copia a JSON
        QJsonArray arr;
        for (const auto& c : m_clients) {
            arr.append(c.toJson());
        }
        QFile f("clientes.json");
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(QJsonDocument(arr).toJson());
            f.close();
        }
        return true;
    }
    return false;
}

bool ClientService::loadClients(const QString& jsonPath, const QString& folderPath) {
    m_clients.clear();

    // 1. Si existe clientes.xlsx y es más nuevo que clientes.json, cargar de Excel
    QString excelPath = "clientes.xlsx";
    if (QFile::exists(excelPath)) {
        QFileInfo xlInfo(excelPath);
        QFileInfo jsonInfo(jsonPath);
        if (!jsonInfo.exists() || xlInfo.lastModified() > jsonInfo.lastModified()) {
            if (loadFromExcel(excelPath)) {
                return true;
            }
        }
    }

    // 2. Si no existe clientes.json pero existe CARPETA CLIENTES, extraer
    if (!QFile::exists(jsonPath) && QDir(folderPath).exists()) {
        scanFolder(folderPath);
    }

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (QFile::exists(excelPath)) {
            return loadFromExcel(excelPath);
        }
        qWarning() << "No se pudo abrir clientes.json, extrayendo desde CARPETA CLIENTES...";
        scanFolder(folderPath);
        return !m_clients.isEmpty();
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray()) {
        QJsonArray arr = doc.array();
        for (const auto& val : arr) {
            if (val.isObject()) {
                m_clients.append(Customer::fromJson(val.toObject()));
            }
        }
    }

    qDebug() << "Cargados con éxito" << m_clients.size() << "clientes desde" << jsonPath;
    return !m_clients.isEmpty();
}

const QVector<Customer>& ClientService::getAllClients() const {
    return m_clients;
}

QVector<Customer> ClientService::search(const QString& text) const {
    if (text.trimmed().isEmpty()) {
        return m_clients;
    }

    QString normText = stripAccents(text.trimmed());
    QStringList tokens = normText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    QVector<Customer> results;
    for (const auto& c : m_clients) {
        QString aliasNorm = stripAccents(c.alias);
        QString nombreNorm = stripAccents(c.nombre);
        QString cifNorm = stripAccents(c.cifNif);
        QString dirNorm = stripAccents(c.direccion);
        QString pobNorm = stripAccents(c.poblacion);

        bool allMatch = true;
        for (const auto& tok : tokens) {
            if (!aliasNorm.contains(tok) && 
                !nombreNorm.contains(tok) && 
                !cifNorm.contains(tok) &&
                !dirNorm.contains(tok) &&
                !pobNorm.contains(tok)) {
                allMatch = false;
                break;
            }
        }

        if (allMatch) {
            results.append(c);
        }
    }

    return results;
}

Customer ClientService::findByAlias(const QString& alias) const {
    for (const auto& c : m_clients) {
        if (c.alias.compare(alias, Qt::CaseInsensitive) == 0 ||
            c.nombre.compare(alias, Qt::CaseInsensitive) == 0) {
            return c;
        }
    }
    return Customer();
}

Customer ClientService::findByName(const QString& name) const {
    return findByAlias(name);
}

bool ClientService::scanFolder(const QString& folderPath, const QString& outputJson) {
    QString scriptPath = "extract_clients.py";
    if (QFile::exists(scriptPath)) {
        QString pythonBin = "/home/enrique/anaconda3/bin/python";
        if (!QFile::exists(pythonBin)) pythonBin = "python3";

        qDebug() << "Escaneando CARPETA CLIENTES para actualizar lista...";
        QProcess process;
        process.start(pythonBin, QStringList() << scriptPath << folderPath << outputJson);
        process.waitForFinished(15000);
        qDebug() << "Extracción de clientes completada.";
        return QFile::exists(outputJson);
    }
    return false;
}
