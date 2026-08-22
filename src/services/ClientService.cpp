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

Customer ClientService::parseClientFromXls(const QString& filePath, const QString& alias) {
    Customer c;
    c.alias = alias.trimmed();
    c.nombre = alias.trimmed();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return c;
    }
    QByteArray data = file.readAll();
    file.close();

    // 1. Buscar posiciones de inicio BOF de BIFF (0x0809)
    QVector<int> bofPositions;
    int searchIdx = 0;
    while (true) {
        int idx = data.indexOf("\x09\x08", searchIdx);
        if (idx == -1) break;
        bofPositions.append(idx);
        searchIdx = idx + 2;
    }

    QMap<QPair<int, int>, QString> cellStrings;

    for (int startPos : bofPositions) {
        int pos = startPos;
        QVector<QString> sstStrings;

        while (pos + 4 <= data.size()) {
            quint16 recType = *reinterpret_cast<const quint16*>(data.constData() + pos);
            quint16 recLen = *reinterpret_cast<const quint16*>(data.constData() + pos + 2);
            pos += 4;
            if (pos + recLen > data.size()) break;
            const char* recData = data.constData() + pos;
            pos += recLen;

            // SST record 0x00FC (Shared String Table)
            if (recType == 0x00FC && recLen >= 8) {
                quint32 numUnique = *reinterpret_cast<const quint32*>(recData + 4);
                int p = 8;
                for (quint32 i = 0; i < numUnique; ++i) {
                    if (p + 3 > recLen) break;
                    quint16 cch = *reinterpret_cast<const quint16*>(recData + p);
                    quint8 flags = *reinterpret_cast<const quint8*>(recData + p + 2);
                    p += 3;
                    bool isUnicode = (flags & 0x01) != 0;
                    bool hasRich = (flags & 0x08) != 0;
                    bool hasPhonetic = (flags & 0x04) != 0;
                    quint16 richCount = 0;
                    quint32 phoneticSize = 0;
                    if (hasRich) {
                        if (p + 2 <= recLen) {
                            richCount = *reinterpret_cast<const quint16*>(recData + p);
                            p += 2;
                        }
                    }
                    if (hasPhonetic) {
                        if (p + 4 <= recLen) {
                            phoneticSize = *reinterpret_cast<const quint32*>(recData + p);
                            p += 4;
                        }
                    }
                    int byteLen = isUnicode ? (cch * 2) : cch;
                    if (p + byteLen > recLen) break;
                    QString str;
                    if (isUnicode) {
                        str = QString::fromUtf16(reinterpret_cast<const char16_t*>(recData + p), cch);
                    } else {
                        str = QString::fromLatin1(recData + p, cch);
                    }
                    p += byteLen;
                    p += richCount * 4;
                    p += phoneticSize;
                    sstStrings.append(str.trimmed());
                }
            }
            // LABELSST record 0x00FD
            else if (recType == 0x00FD && recLen >= 10) {
                quint16 row = *reinterpret_cast<const quint16*>(recData);
                quint16 col = *reinterpret_cast<const quint16*>(recData + 2);
                quint32 sstIdx = *reinterpret_cast<const quint32*>(recData + 6);
                if (static_cast<int>(sstIdx) < sstStrings.size()) {
                    cellStrings[qMakePair(static_cast<int>(row), static_cast<int>(col))] = sstStrings[sstIdx];
                }
            }
            // LABEL record 0x0004 o 0x0204
            else if ((recType == 0x0004 || recType == 0x0204) && recLen >= 8) {
                quint16 row = *reinterpret_cast<const quint16*>(recData);
                quint16 col = *reinterpret_cast<const quint16*>(recData + 2);
                quint16 cch = *reinterpret_cast<const quint16*>(recData + 6);
                if (recLen >= 8 + cch) {
                    QString str = QString::fromLatin1(recData + 8, cch).trimmed();
                    cellStrings[qMakePair(static_cast<int>(row), static_cast<int>(col))] = str;
                }
            }
        }
    }

    // Extraer campos a partir de cellStrings
    for (auto it = cellStrings.begin(); it != cellStrings.end(); ++it) {
        int r = it.key().first;
        int col = it.key().second;
        QString val = it.value();
        QString valLow = stripAccents(val);

        auto getNextVal = [&](int rowIdx, int colIdx) -> QString {
            if (cellStrings.contains(qMakePair(rowIdx, colIdx + 1))) {
                QString v = cellStrings[qMakePair(rowIdx, colIdx + 1)].trimmed();
                if (!v.isEmpty()) return v;
            }
            if (cellStrings.contains(qMakePair(rowIdx, colIdx + 2))) {
                QString v = cellStrings[qMakePair(rowIdx, colIdx + 2)].trimmed();
                if (!v.isEmpty()) return v;
            }
            return "";
        };

        if (valLow == "nombre" || valLow == "nombre:") {
            if (c.nombre == alias || c.nombre.isEmpty()) {
                QString v = getNextVal(r, col);
                if (!v.isEmpty()) c.nombre = v;
            }
        } else if (valLow.contains("direcci") || valLow.contains("domicilio")) {
            if (c.direccion.isEmpty()) {
                QString v = getNextVal(r, col);
                if (!v.isEmpty()) c.direccion = v;
            }
        } else if (valLow.contains("poblaci")) {
            if (c.poblacion.isEmpty()) {
                QString v = getNextVal(r, col);
                if (!v.isEmpty()) c.poblacion = v;
            }
        } else if (valLow.contains("provincia")) {
            if (c.provincia.isEmpty()) {
                QString v = getNextVal(r, col);
                if (!v.isEmpty()) c.provincia = v;
            }
        } else if ((valLow.contains("cif") || valLow.contains("nif") || valLow.contains("d.n.i")) && !val.contains("52434449")) {
            if (c.cifNif.isEmpty()) {
                QString v = getNextVal(r, col);
                if (!v.isEmpty() && !v.contains("52434449")) c.cifNif = v;
            }
        }
    }

    // Fallback: Si no se encontró celda pero hay texto plano
    if (c.nombre.isEmpty() || c.nombre == alias) {
        QRegularExpression rx("[\\x20-\\x7E\\xA0-\\xFF]{3,}");
        auto matchIt = rx.globalMatch(QString::fromLatin1(data));
        QStringList strings;
        while (matchIt.hasNext()) {
            strings.append(matchIt.next().captured().trimmed());
        }
        for (int i = 0; i < strings.size(); ++i) {
            QString sLow = stripAccents(strings[i]);
            if (sLow == "nombre" && i + 1 < strings.size() && c.nombre == alias) {
                c.nombre = strings[i + 1];
            } else if (sLow.contains("direcci") && i + 1 < strings.size() && c.direccion.isEmpty()) {
                c.direccion = strings[i + 1];
            } else if (sLow.contains("poblaci") && i + 1 < strings.size() && c.poblacion.isEmpty()) {
                c.poblacion = strings[i + 1];
            } else if (sLow.contains("provincia") && i + 1 < strings.size() && c.provincia.isEmpty()) {
                c.provincia = strings[i + 1];
            } else if ((sLow.contains("cif") || sLow.contains("nif")) && i + 1 < strings.size() && c.cifNif.isEmpty() && !strings[i].contains("52434449")) {
                c.cifNif = strings[i + 1];
            }
        }
    }

    if (c.nombre.isEmpty() || c.nombre.contains("Juan Manuel", Qt::CaseInsensitive) || c.nombre == "DATOS DEL CLIENTE") {
        c.nombre = c.alias;
    }

    return c;
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

            auto getNextVal = [&](int cIdx) -> QString {
                for (int nextCol = cIdx + 1; nextCol < row.size(); ++nextCol) {
                    QString val = row[nextCol].trimmed();
                    if (!val.isEmpty()) return val;
                }
                return "";
            };

            if ((cellStr == "nombre" || cellStr == "nombre:" || cellStr.startsWith("nombre ")) && (c.nombre == alias || c.nombre.isEmpty())) {
                QString v = getNextVal(col);
                if (!v.isEmpty()) c.nombre = v;
            } else if ((cellStr.contains("direcci") || cellStr.contains("domicilio")) && c.direccion.isEmpty()) {
                QString v = getNextVal(col);
                if (!v.isEmpty()) c.direccion = v;
            } else if (cellStr.contains("poblaci") && c.poblacion.isEmpty()) {
                QString v = getNextVal(col);
                if (!v.isEmpty()) c.poblacion = v;
            } else if (cellStr.contains("provincia") && c.provincia.isEmpty()) {
                QString v = getNextVal(col);
                if (!v.isEmpty()) c.provincia = v;
            } else if ((cellStr.contains("cif") || cellStr.contains("nif") || cellStr.contains("d.n.i")) && c.cifNif.isEmpty()) {
                QString v = getNextVal(col);
                if (!v.isEmpty() && !v.contains("52434449")) c.cifNif = v;
            } else if ((cellStr.contains("telefono") || cellStr.contains("tfno") || cellStr.contains("tel.")) && c.telefono.isEmpty()) {
                QString v = getNextVal(col);
                if (!v.isEmpty()) c.telefono = v;
            } else if ((cellStr.contains("email") || cellStr.contains("correo")) && c.email.isEmpty()) {
                QString v = getNextVal(col);
                if (!v.isEmpty()) c.email = v;
            }
        }
    }

    if (c.nombre.isEmpty() || c.nombre.contains("Juan Manuel", Qt::CaseInsensitive) || c.nombre == "DATOS DEL CLIENTE") {
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

void ClientService::addOrUpdateClient(const Customer& c) {
    QString key = stripAccents(c.alias.isEmpty() ? c.nombre : c.alias);
    if (key.isEmpty()) return;

    bool found = false;
    for (int i = 0; i < m_clients.size(); ++i) {
        QString itemKey = stripAccents(m_clients[i].alias.isEmpty() ? m_clients[i].nombre : m_clients[i].alias);
        if (itemKey == key || (!c.cifNif.isEmpty() && m_clients[i].cifNif.compare(c.cifNif, Qt::CaseInsensitive) == 0)) {
            m_clients[i] = c;
            found = true;
            break;
        }
    }

    if (!found) {
        m_clients.append(c);
        std::sort(m_clients.begin(), m_clients.end(), [](const Customer& a, const Customer& b) {
            QString nameA = a.alias.isEmpty() ? a.nombre : a.alias;
            QString nameB = b.alias.isEmpty() ? b.nombre : b.alias;
            return nameA.localeAwareCompare(nameB) < 0;
        });
    }

    saveToJson("clientes.json");
}

bool ClientService::deleteClient(const QString& aliasOrName) {
    QString key = stripAccents(aliasOrName);
    for (int i = 0; i < m_clients.size(); ++i) {
        QString aKey = stripAccents(m_clients[i].alias);
        QString nKey = stripAccents(m_clients[i].nombre);
        if (aKey == key || nKey == key) {
            m_clients.removeAt(i);
            saveToJson("clientes.json");
            return true;
        }
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

            Customer parsed;
            if (fi.suffix().compare("xlsx", Qt::CaseInsensitive) == 0) {
                parsed = parseClientFromXlsx(fi.absoluteFilePath(), alias);
            } else {
                parsed = parseClientFromXls(fi.absoluteFilePath(), alias);
            }

            if (!parsed.nombre.isEmpty()) {
                // Si el cliente ya existía pero el archivo tiene campos más completos, actualizarlos
                if (clientMap.contains(key)) {
                    Customer& existing = clientMap[key];
                    if (existing.nombre.isEmpty() || existing.nombre == alias) existing.nombre = parsed.nombre;
                    if (existing.cifNif.isEmpty()) existing.cifNif = parsed.cifNif;
                    if (existing.direccion.isEmpty()) existing.direccion = parsed.direccion;
                    if (existing.poblacion.isEmpty()) existing.poblacion = parsed.poblacion;
                    if (existing.provincia.isEmpty()) existing.provincia = parsed.provincia;
                } else {
                    clientMap[key] = parsed;
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
