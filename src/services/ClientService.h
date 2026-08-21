#pragma once

#include <QString>
#include <QVector>
#include "../models/Invoice.h"

class ClientService {
public:
    static ClientService& instance();

    bool loadClients(const QString& jsonPath = "clientes.json", const QString& folderPath = "CARPETA CLIENTES");
    const QVector<Customer>& getAllClients() const;
    QVector<Customer> search(const QString& query) const;
    Customer findByName(const QString& name) const;

private:
    ClientService() = default;
    void scanFolder(const QString& folderPath);

    QVector<Customer> m_clients;
};
