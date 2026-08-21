#include "ClientService.h"
#include <QFile>
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

bool ClientService::loadClients(const QString& jsonPath, const QString& folderPath) {
    m_clients.clear();

    // Si no existe clientes.json pero existe CARPETA CLIENTES, extraer
    if (!QFile::exists(jsonPath) && QDir(folderPath).exists()) {
        scanFolder(folderPath);
    }

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
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

void ClientService::scanFolder(const QString& folderPath) {
    QString scriptPath = "extract_clients.py";
    if (QFile::exists(scriptPath)) {
        QString pythonBin = "/home/enrique/anaconda3/bin/python";
        if (!QFile::exists(pythonBin)) pythonBin = "python3";

        QProcess process;
        process.start(pythonBin, QStringList() << scriptPath << folderPath << "clientes.json");
        process.waitForFinished(10000);
    }
}

const QVector<Customer>& ClientService::getAllClients() const {
    return m_clients;
}

QVector<Customer> ClientService::search(const QString& query) const {
    QVector<Customer> results;
    QString cleanQuery = stripAccents(query).trimmed();
    QStringList terms = cleanQuery.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (terms.isEmpty()) {
        return m_clients;
    }

    for (const auto& c : m_clients) {
        // Incluir alias, nombre fiscal, archivo, CIF, dirección, población y provincia en el índice de búsqueda
        QString fullText = stripAccents(c.alias + " " + c.nombre + " " + c.file + " " + c.cifNif + " " + c.direccion + " " + c.poblacion + " " + c.provincia);
        
        bool match = true;
        for (const auto& term : terms) {
            if (!fullText.contains(term)) {
                match = false;
                break;
            }
        }
        if (match) {
            results.append(c);
        }
    }
    return results;
}

Customer ClientService::findByName(const QString& name) const {
    QString target = stripAccents(name).trimmed();
    for (const auto& c : m_clients) {
        if (stripAccents(c.nombre).trimmed() == target || 
            stripAccents(c.alias).trimmed() == target ||
            stripAccents(c.file).trimmed() == target) {
            return c;
        }
    }
    // Buscar si target contiene alias o nombre
    for (const auto& c : m_clients) {
        if (!c.alias.isEmpty() && target.contains(stripAccents(c.alias))) {
            return c;
        }
        if (!c.nombre.isEmpty() && target.contains(stripAccents(c.nombre))) {
            return c;
        }
    }
    return Customer();
}
