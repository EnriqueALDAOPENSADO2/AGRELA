#pragma once

#include <QVector>
#include <QString>
#include "../models/Invoice.h"

class ClientService {
public:
    static ClientService& instance();

    bool loadClients(const QString& jsonPath = "clientes.json", const QString& folderPath = "CARPETA CLIENTES");
    bool loadFromExcel(const QString& xlsxPath = "clientes.xlsx");
    const QVector<Customer>& getAllClients() const;
    QVector<Customer> search(const QString& text) const;
    Customer findByAlias(const QString& alias) const;
    Customer findByName(const QString& name) const;

    bool scanFolder(const QString& folderPath = "CARPETA CLIENTES", const QString& outputJson = "clientes.json");

private:
    ClientService() = default;
    QVector<Customer> m_clients;
};
