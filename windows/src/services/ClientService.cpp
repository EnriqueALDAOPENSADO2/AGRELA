#include "ClientService.h"
#include "../utils/ExcelReader.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

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

Customer ClientService::parseClientFromXlsx(const QString& filePath, const QString& alias) {
    Customer c;
    c.alias = alias.trimmed();
    c.nombre = alias.trimmed();

    auto rows = ExcelReader::readXlsx(filePath);
    if (rows.isEmpty()) {
        return c;
    }

    for (int r = 0; r < qMin(rows.size(), 25); ++r) {
        const auto& row = rows[r];
        for (int col = 0; col < row.size(); ++col) {
            QString cellStr = stripAccents(row[col].trimmed());
            
            // Buscar etiquetas y extraer valor de la siguiente columna con texto
            if (cellStr.contains("nombre") && (c.nombre == alias || c.nombre.isEmpty())) {
                for (int nextCol = col + 1; nextCol < row.size(); ++nextCol) {
                    QString val = row[nextCol].trimmed();
                    if (!val.isEmpty()) {
                        c.nombre = val;
                        break;
                    }
                }
            } else if ((cellStr.contains("direcci") || cellStr.contains("domicilio")) && c.direccion.isEmpty()) {
                for (int nextCol = col + 1; nextCol < row.size(); ++nextCol) {
                    QString val = row[nextCol].trimmed();
                    if (!val.isEmpty()) {
                        c.direccion = val;
                        break;
                    }
                }
            } else if (cellStr.contains("poblaci") && c.poblacion.isEmpty()) {
                for (int nextCol = col + 1; nextCol < row.size(); ++nextCol) {
                    QString val = row[nextCol].trimmed();
                    if (!val.isEmpty()) {
                        c.poblacion = val;
                        break;
                    }
                }
            } else if (cellStr.contains("provincia") && c.provincia.isEmpty()) {
                for (int nextCol = col + 1; nextCol < row.size(); ++nextCol) {
                    QString val = row[nextCol].trimmed();
                    if (!val.isEmpty()) {
                        c.provincia = val;
                        break;
                    }
                }
            } else if ((cellStr.contains("cif") || cellStr.contains("nif") || cellStr.contains("d.n.i")) && c.cifNif.isEmpty()) {
                for (int nextCol = col + 1; nextCol < row.size(); ++nextCol) {
                    QString val = row[nextCol].trimmed();
                    if (!val.isEmpty() && !val.contains("52434449")) { // evitar DNI propio de cabecera
                        c.cifNif = val;
                        break;
                    }
                }
            } else if ((cellStr.contains("telefono") || cellStr.contains("tfno") || cellStr.contains("tel.")) && c.telefono.isEmpty()) {
                for (int nextCol = col + 1; nextCol < row.size(); ++nextCol) {
                    QString val = row[nextCol].trimmed();
                    if (!val.isEmpty()) {
                        c.telefono = val;
                        break;
                    }
                }
            } else if ((cellStr.contains("email") || cellStr.contains("correo")) && c.email.isEmpty()) {
                for (int nextCol = col + 1; nextCol < row.size(); ++nextCol) {
                    QString val = row[nextCol].trimmed();
                    if (!val.isEmpty()) {
                        c.email = val;
                        break;
                    }
                }
            }
        }
    }

    if (c.nombre.isEmpty() || c.nombre.contains("Juan Manuel", Qt::CaseInsensitive) || c.nombre.contains("DATOS DEL CLIENTE", Qt::CaseInsensitive)) {
        c.nombre = c.alias;
    }

    return c;
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
        saveToJson("clientes.json");
        return true;
    }
    return false;
}

bool ClientService::saveToJson(const QString& jsonPath) {
    QJsonArray arr;
    for (const auto& c : m_clients) {
        arr.append(c.toJson());
    }
    QFile f(jsonPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(QJsonDocument(arr).toJson());
        f.close();
        return true;
    }
    return false;
}

bool ClientService::syncFolder(const QString& folderPath, const QString& outputJson) {
    QMap<QString, Customer> clientMap;

    // 1. Cargar clientes existentes en memoria o clientes.json para preservar datos
    for (const auto& c : m_clients) {
        QString key = stripAccents(c.alias.isEmpty() ? c.nombre : c.alias);
        if (!key.isEmpty()) {
            clientMap[key] = c;
        }
    }

    // 2. Si existe clientes.xlsx, leerlo
    QString excelPath = "clientes.xlsx";
    if (QFile::exists(excelPath)) {
        auto rows = ExcelReader::readXlsx(excelPath);
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

            QString key = stripAccents(c.alias.isEmpty() ? c.nombre : c.alias);
            if (!key.isEmpty()) {
                clientMap[key] = c;
            }
        }
    }

    // 3. Escaneo nativo ultra-rápido de CARPETA CLIENTES (C++ puro, 0 Python)
    QDir dir(folderPath);
    if (dir.exists()) {
        QFileInfoList files = dir.entryInfoList(QStringList() << "*.xlsx" << "*.xls", QDir::Files, QDir::Name);
        for (const auto& fi : files) {
            QString alias = fi.completeBaseName();
            QString key = stripAccents(alias);

            if (fi.suffix().compare("xlsx", Qt::CaseInsensitive) == 0) {
                Customer parsed = parseClientFromXlsx(fi.absoluteFilePath(), alias);
                if (!parsed.nombre.isEmpty()) {
                    clientMap[key] = parsed;
                }
            } else {
                if (!clientMap.contains(key)) {
                    Customer c;
                    c.alias = alias;
                    c.nombre = alias;
                    clientMap[key] = c;
                }
            }
        }
    }

    m_clients = clientMap.values().toVector();

    // Ordenar alfabéticamente por alias o nombre
    std::sort(m_clients.begin(), m_clients.end(), [](const Customer& a, const Customer& b) {
        QString nameA = a.alias.isEmpty() ? a.nombre : a.alias;
        QString nameB = b.alias.isEmpty() ? b.nombre : b.alias;
        return nameA.localeAwareCompare(nameB) < 0;
    });

    saveToJson(outputJson);
    qDebug() << "Sincronización nativa de clientes completada. Total:" << m_clients.size() << "clientes.";
    return !m_clients.isEmpty();
}

bool ClientService::loadClients(const QString& jsonPath, const QString& folderPath) {
    m_clients.clear();

    // 1. Cargar desde JSON (arranque instantáneo)
    QFile file(jsonPath);
    bool loaded = false;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
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
            if (!m_clients.isEmpty()) {
                loaded = true;
            }
        }
    }

    // 2. Si no había datos o clientes.xlsx es más reciente, sincronizar
    QString excelPath = "clientes.xlsx";
    if (QFile::exists(excelPath)) {
        QFileInfo xlInfo(excelPath);
        QFileInfo jsonInfo(jsonPath);
        if (!loaded || !jsonInfo.exists() || xlInfo.lastModified() > jsonInfo.lastModified()) {
            if (loadFromExcel(excelPath)) {
                loaded = true;
            }
        }
    }

    // 3. Si sigue sin datos, hacer escaneo nativo de la carpeta
    if (m_clients.isEmpty()) {
        syncFolder(folderPath, jsonPath);
    }

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
        QString provNorm = stripAccents(c.provincia);

        bool allMatch = true;
        for (const auto& tok : tokens) {
            if (!aliasNorm.contains(tok) && 
                !nombreNorm.contains(tok) && 
                !cifNorm.contains(tok) &&
                !dirNorm.contains(tok) &&
                !pobNorm.contains(tok) &&
                !provNorm.contains(tok)) {
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
    for (const auto& c : m_clients) {
        if (c.nombre.compare(name, Qt::CaseInsensitive) == 0 ||
            c.alias.compare(name, Qt::CaseInsensitive) == 0) {
            return c;
        }
        QString combined = QString("%1 (%2)").arg(c.alias, c.nombre);
        if (combined.compare(name, Qt::CaseInsensitive) == 0) {
            return c;
        }
    }
    return findByAlias(name);
}
