#pragma once

#include <QVector>
#include <QString>
#include <QMap>
#include "../models/Invoice.h"

class ClientService {
public:
    static ClientService& instance();

    bool loadClients(const QString& jsonPath = "clientes.json", const QString& folderPath = "CARPETA CLIENTES");
    bool loadFromExcel(const QString& xlsxPath = "clientes.xlsx");
    bool saveToJson(const QString& jsonPath = "clientes.json");

    const QVector<Customer>& getAllClients() const;
    QVector<Customer> search(const QString& text) const;
    Customer findByAlias(const QString& alias) const;
    Customer findByName(const QString& name) const;

    void addOrUpdateClient(const Customer& c);
    bool deleteClient(const QString& aliasOrName);

    bool syncFolder(const QString& folderPath = "CARPETA CLIENTES", const QString& outputJson = "clientes.json");
    static Customer parseClientFromXlsx(const QString& filePath, const QString& alias);
    static Customer parseClientFromXls(const QString& filePath, const QString& alias);

private:
    ClientService() = default;
    QVector<Customer> m_clients;
};
