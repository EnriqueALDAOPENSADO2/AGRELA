#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include "../models/TransactionItem.h"

class PurchasesWidget : public QWidget {
    Q_OBJECT

public:
    explicit PurchasesWidget(QWidget* parent = nullptr);
    void refreshPurchases();

private slots:
    void onSearchChanged();
    void onAddPurchaseClicked();
    void onDeletePurchaseClicked();
    void onExportRegisterClicked();

private:
    void setupUi();
    void populateTable(const QVector<PurchaseRecord>& purchases);
    void clearForm();

    // Formulario de entrada
    QDateEdit* m_dateEdit = nullptr;
    QLineEdit* m_txtProveedor = nullptr;
    QLineEdit* m_txtCifNif = nullptr;
    QLineEdit* m_txtNumFacturaProv = nullptr;
    QLineEdit* m_txtConcepto = nullptr;
    QDoubleSpinBox* m_spnUnidades = nullptr;
    QDoubleSpinBox* m_spnPrecioUnitario = nullptr;
    QComboBox* m_cmbTipoIva = nullptr;
    QPushButton* m_btnAddPurchase = nullptr;

    // Resumen y Filtros
    QLineEdit* m_searchEdit = nullptr;
    QLabel* m_lblCount = nullptr;
    QLabel* m_lblTotalBruto = nullptr;
    QLabel* m_lblTotalIva = nullptr;
    QLabel* m_lblTotalFacturado = nullptr;
    QTableWidget* m_table = nullptr;
    QPushButton* m_btnExportExcel = nullptr;

    QVector<PurchaseRecord> m_currentPurchases;
};
