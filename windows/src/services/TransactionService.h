#pragma once

#include <QVector>
#include <QString>
#include "../models/TransactionItem.h"

class TransactionService {
public:
    static TransactionService& instance();

    bool loadTransactions(const QString& salesJson = "ventas.json", const QString& purchasesJson = "compras.json");
    bool saveSales(const QString& salesJson = "ventas.json");
    bool savePurchases(const QString& purchasesJson = "compras.json");

    void recordSale(const Invoice& invoice, const QString& excelPath = "", const QString& pdfPath = "");
    void addPurchase(const PurchaseRecord& purchase);
    bool removeSale(const QString& id);
    bool removePurchase(const QString& id);

    const QVector<SaleRecord>& getSales() const;
    const QVector<PurchaseRecord>& getPurchases() const;

    double getTotalVentasBruto() const;
    double getTotalVentasIva() const;
    double getTotalVentasFacturado() const;

    double getTotalComprasBase() const;
    double getTotalComprasIva() const;
    double getTotalComprasFacturado() const;

    bool exportToExcel(const QString& outputPath = "registro_compras_ventas.xlsx");

private:
    TransactionService() = default;

    QVector<SaleRecord> m_sales;
    QVector<PurchaseRecord> m_purchases;
    QString m_salesPath = "ventas.json";
    QString m_purchasesPath = "compras.json";
};
