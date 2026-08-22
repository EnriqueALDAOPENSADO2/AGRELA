#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QCompleter>
#include "../models/Invoice.h"
#include "CatalogWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onCatalogItemSelected(const CatalogItem& item);
    void onAddCustomLine();
    void onEditSelectedLine();
    void onDeleteSelectedLine();
    void onMoveLineUp();
    void onMoveLineDown();
    void onLineDoubleClicked(int row, int col);

    void onBrowseClientsClicked();
    void onClientCompleterActivated(const QString& name);

    void onTarifaChanged(int index);
    void onFormaPagoChanged(const QString& formaPago);
    void onFechaEmisionChanged(const QDate& date);

    void onBrowseOutputDirClicked();
    void onGenerateBothClicked();
    void onExportExcelOnly();
    void onExportPdfOnly();
    void onNewInvoice();
    void onSaveInvoiceJson();
    void onLoadInvoiceJson();

    void updateTotals();
    void refreshInvoiceTable();

private:
    void setupUi();
    void setupHeader();
    void setupClientAndInvoiceMeta();
    void setupLinesTable();
    void setupTotalsCard();
    void setupActionsBar();
    void updateClientCompleter();

    Invoice getInvoiceFromUi() const;
    void loadInvoiceToUi(const Invoice& inv);
    void applyCustomerData(const Customer& c);

    // Widgets de Cliente
    QLineEdit* m_txtClientNombre = nullptr;
    QLineEdit* m_txtClientCif = nullptr;
    QLineEdit* m_txtClientDireccion = nullptr;
    QLineEdit* m_txtClientPoblacion = nullptr;
    QLineEdit* m_txtClientProvincia = nullptr;
    QLineEdit* m_txtClientTelefono = nullptr;
    QCompleter* m_clientCompleter = nullptr;

    // Widgets de Factura
    QLineEdit* m_txtNumFactura = nullptr;
    QDateEdit* m_dateEdit = nullptr;
    QDateEdit* m_dateVencimiento = nullptr;
    QComboBox* m_cmbFormaPago = nullptr;
    QComboBox* m_cmbTarifa = nullptr;

    // Selector de Directorio de Guardado
    QLineEdit* m_txtOutputDir = nullptr;

    // Tabla de Líneas
    QTableWidget* m_tableLines = nullptr;
    CatalogWidget* m_catalogWidget = nullptr;

    // Totales
    QLabel* m_lblTotalBruto = nullptr;
    QDoubleSpinBox* m_spnTipoIva = nullptr;
    QLabel* m_lblCuotaIva = nullptr;
    QLabel* m_lblTotalFactura = nullptr;

    Invoice m_currentInvoice;
};
