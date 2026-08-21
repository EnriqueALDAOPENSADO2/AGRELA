#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include "../models/TransactionItem.h"

class SalesWidget : public QWidget {
    Q_OBJECT

public:
    explicit SalesWidget(QWidget* parent = nullptr);
    void refreshSales();

private slots:
    void onSearchChanged();
    void onOpenExcelClicked();
    void onOpenPdfClicked();
    void onExportRegisterClicked();
    void onDeleteSaleClicked();

private:
    void setupUi();
    void populateTable(const QVector<SaleRecord>& sales);

    QLineEdit* m_searchEdit = nullptr;
    QLabel* m_lblCount = nullptr;
    QLabel* m_lblTotalBruto = nullptr;
    QLabel* m_lblTotalIva = nullptr;
    QLabel* m_lblTotalFacturado = nullptr;
    QTableWidget* m_table = nullptr;
    QPushButton* m_btnExportExcel = nullptr;

    QVector<SaleRecord> m_currentSales;
};
