#include "PurchasesWidget.h"
#include "../services/TransactionService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QRegularExpression>

static QString removeAccents(const QString& str) {
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

PurchasesWidget::PurchasesWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
    refreshPurchases();
}

void PurchasesWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // 1. Formulario de Alta de Compras
    auto* formGroup = new QGroupBox("🛒 Registrar Nueva Compra / Gasto a Proveedor", this);
    formGroup->setStyleSheet(
        "QGroupBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; margin-top: 6px; font-weight: bold; color: #1F4E78; font-size: 13px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 14px; padding: 0 6px; background-color: #FFFFFF; }"
        "QLineEdit, QDateEdit, QComboBox, QDoubleSpinBox { background-color: #FFFFFF; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 5px; padding: 5px 8px; font-size: 12px; }"
        "QLineEdit:focus, QDateEdit:focus, QComboBox:focus, QDoubleSpinBox:focus { border: 1.5px solid #2B78C5; }"
    );

    auto* formGrid = new QGridLayout(formGroup);
    formGrid->setContentsMargins(12, 14, 12, 12);
    formGrid->setHorizontalSpacing(12);
    formGrid->setVerticalSpacing(8);

    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd/MM/yyyy");

    m_txtProveedor = new QLineEdit(this);
    m_txtProveedor->setPlaceholderText("Nombre / Razón Social del Proveedor");

    m_txtCifNif = new QLineEdit(this);
    m_txtCifNif->setPlaceholderText("CIF / NIF Proveedor");

    m_txtNumFacturaProv = new QLineEdit(this);
    m_txtNumFacturaProv->setPlaceholderText("Nº Factura del Proveedor");

    m_txtConcepto = new QLineEdit(this);
    m_txtConcepto->setPlaceholderText("Descripción / Material / Artículo Comprado");

    m_spnUnidades = new QDoubleSpinBox(this);
    m_spnUnidades->setRange(0.01, 99999.0);
    m_spnUnidades->setValue(1.0);

    m_spnPrecioUnitario = new QDoubleSpinBox(this);
    m_spnPrecioUnitario->setRange(0.0, 999999.0);
    m_spnPrecioUnitario->setDecimals(2);
    m_spnPrecioUnitario->setSuffix(" €");

    m_cmbTipoIva = new QComboBox(this);
    m_cmbTipoIva->addItem("21 % (General)", 0.21);
    m_cmbTipoIva->addItem("10 % (Reducido)", 0.10);
    m_cmbTipoIva->addItem("4 % (Superreducido)", 0.04);
    m_cmbTipoIva->addItem("0 % (Exento)", 0.00);

    m_btnAddPurchase = new QPushButton("➕ Registrar Compra", this);
    m_btnAddPurchase->setCursor(Qt::PointingHandCursor);
    m_btnAddPurchase->setStyleSheet(
        "QPushButton { background-color: #2B78C5; color: #FFFFFF; font-weight: bold; font-size: 13px; padding: 7px 16px; border-radius: 6px; border: none; }"
        "QPushButton:hover { background-color: #1F5F9F; }"
    );

    // Fila 1: Fecha, Proveedor, CIF/NIF, Nº Factura Prov
    formGrid->addWidget(new QLabel("Fecha Compra:", this), 0, 0);
    formGrid->addWidget(m_dateEdit, 0, 1);
    formGrid->addWidget(new QLabel("Proveedor:", this), 0, 2);
    formGrid->addWidget(m_txtProveedor, 0, 3);
    formGrid->addWidget(new QLabel("CIF/NIF:", this), 0, 4);
    formGrid->addWidget(m_txtCifNif, 0, 5);

    // Fila 2: Nº Factura Prov, Concepto, Uds, Precio Unit., IVA, Botón
    formGrid->addWidget(new QLabel("Nº Factura Prov.:", this), 1, 0);
    formGrid->addWidget(m_txtNumFacturaProv, 1, 1);
    formGrid->addWidget(new QLabel("Concepto / Material:", this), 1, 2);
    formGrid->addWidget(m_txtConcepto, 1, 3);
    formGrid->addWidget(new QLabel("Uds:", this), 1, 4);
    formGrid->addWidget(m_spnUnidades, 1, 5);

    // Fila 3: Precio, Tipo IVA, Botón Añadir
    auto* subRowLayout = new QHBoxLayout();
    subRowLayout->addWidget(new QLabel("Precio Unit.:", this));
    subRowLayout->addWidget(m_spnPrecioUnitario);
    subRowLayout->addWidget(new QLabel("IVA:", this));
    subRowLayout->addWidget(m_cmbTipoIva);
    subRowLayout->addStretch();
    subRowLayout->addWidget(m_btnAddPurchase);

    formGrid->addLayout(subRowLayout, 2, 0, 1, 6);

    mainLayout->addWidget(formGroup);

    // 2. Tarjetas de Resumen Financiero de Compras
    auto* summaryCard = new QFrame(this);
    summaryCard->setStyleSheet("background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; padding: 10px;");
    auto* summaryLayout = new QHBoxLayout(summaryCard);
    summaryLayout->setSpacing(20);

    auto makeSummaryBox = [](const QString& title, const QString& initialVal, const QString& colorHex) {
        auto* box = new QVBoxLayout();
        auto* lblT = new QLabel(title);
        lblT->setStyleSheet("font-size: 11px; font-weight: bold; color: #64748B; text-transform: uppercase;");
        auto* lblV = new QLabel(initialVal);
        lblV->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1;").arg(colorHex));
        box->addWidget(lblT);
        box->addWidget(lblV);
        return qMakePair(box, lblV);
    };

    auto b1 = makeSummaryBox("Base Imponible Compras", "0.00 €", "#1E293B");
    auto b2 = makeSummaryBox("Total IVA Soportado", "0.00 €", "#D97706");
    auto b3 = makeSummaryBox("Total Gastos Compras", "0.00 €", "#B91C1C");

    m_lblTotalBruto = b1.second;
    m_lblTotalIva = b2.second;
    m_lblTotalFacturado = b3.second;

    summaryLayout->addLayout(b1.first);
    summaryLayout->setStretchFactor(b1.first, 1);
    summaryLayout->addLayout(b2.first);
    summaryLayout->setStretchFactor(b2.first, 1);
    summaryLayout->addLayout(b3.first);
    summaryLayout->setStretchFactor(b3.first, 1);

    m_btnExportExcel = new QPushButton("📊 Exportar Registro (Excel)", this);
    m_btnExportExcel->setCursor(Qt::PointingHandCursor);
    m_btnExportExcel->setStyleSheet(
        "QPushButton { background-color: #10B981; color: #FFFFFF; font-weight: bold; font-size: 12px; padding: 8px 14px; border-radius: 6px; border: none; }"
        "QPushButton:hover { background-color: #059669; }"
    );
    summaryLayout->addWidget(m_btnExportExcel);

    mainLayout->addWidget(summaryCard);

    // 3. Buscador y Filtros
    auto* filterLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("🔍 Buscar compra (proveedor, CIF, nº factura, concepto)...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 7px 10px; font-size: 13px; }"
        "QLineEdit:focus { border: 1.5px solid #2B78C5; }"
    );
    filterLayout->addWidget(m_searchEdit, 1);

    m_lblCount = new QLabel("0 compras registradas", this);
    m_lblCount->setStyleSheet("font-size: 12px; color: #64748B; font-weight: 500;");
    filterLayout->addWidget(m_lblCount);

    mainLayout->addLayout(filterLayout);

    // 4. Tabla del Historial de Compras
    m_table = new QTableWidget(this);
    m_table->setColumnCount(10);
    m_table->setHorizontalHeaderLabels({
        "Fecha", "Proveedor", "CIF / NIF", "Nº Factura",
        "Concepto", "Uds.", "Precio Unit.", "Base (€)", "IVA (€)", "Total (€)"
    });

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);

    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 6px; gridline-color: #E2E8F0; }"
        "QHeaderView::section { background-color: #2B5B84; color: #FFFFFF; padding: 7px 6px; font-weight: bold; font-size: 12px; border: none; border-right: 1px solid #1F4E78; }"
        "QTableWidget::item { padding: 4px; color: #1E293B; }"
        "QTableWidget::item:selected { background-color: #D9E1F2; color: #1F4E78; font-weight: bold; }"
    );

    mainLayout->addWidget(m_table, 1);

    connect(m_btnAddPurchase, &QPushButton::clicked, this, &PurchasesWidget::onAddPurchaseClicked);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PurchasesWidget::onSearchChanged);
    connect(m_btnExportExcel, &QPushButton::clicked, this, &PurchasesWidget::onExportRegisterClicked);
}

void PurchasesWidget::refreshPurchases() {
    m_currentPurchases = TransactionService::instance().getPurchases();
    
    m_lblTotalBruto->setText(QString("%1 €").arg(TransactionService::instance().getTotalComprasBase(), 0, 'f', 2));
    m_lblTotalIva->setText(QString("%1 €").arg(TransactionService::instance().getTotalComprasIva(), 0, 'f', 2));
    m_lblTotalFacturado->setText(QString("%1 €").arg(TransactionService::instance().getTotalComprasFacturado(), 0, 'f', 2));

    onSearchChanged();
}

void PurchasesWidget::clearForm() {
    m_txtProveedor->clear();
    m_txtCifNif->clear();
    m_txtNumFacturaProv->clear();
    m_txtConcepto->clear();
    m_spnUnidades->setValue(1.0);
    m_spnPrecioUnitario->setValue(0.0);
    m_dateEdit->setDate(QDate::currentDate());
}

void PurchasesWidget::onAddPurchaseClicked() {
    QString prov = m_txtProveedor->text().trimmed();
    QString concepto = m_txtConcepto->text().trimmed();
    double precio = m_spnPrecioUnitario->value();

    if (prov.isEmpty()) {
        QMessageBox::warning(this, "Campo Requerido", "Por favor, introduce el nombre del proveedor.");
        m_txtProveedor->setFocus();
        return;
    }
    if (concepto.isEmpty()) {
        QMessageBox::warning(this, "Campo Requerido", "Por favor, introduce el concepto o material comprado.");
        m_txtConcepto->setFocus();
        return;
    }
    if (precio <= 0.0) {
        QMessageBox::warning(this, "Campo Requerido", "Por favor, introduce un precio unitario mayor a 0.");
        m_spnPrecioUnitario->setFocus();
        return;
    }

    PurchaseRecord p;
    p.fecha = m_dateEdit->date();
    p.proveedor = prov;
    p.cifNif = m_txtCifNif->text().trimmed();
    p.numFacturaProveedor = m_txtNumFacturaProv->text().trimmed();
    p.concepto = concepto;
    p.unidades = m_spnUnidades->value();
    p.precioUnitario = precio;
    p.tipoIva = m_cmbTipoIva->currentData().toDouble();

    TransactionService::instance().addPurchase(p);
    clearForm();
    refreshPurchases();

    QMessageBox::information(this, "Compra Registrada", "La compra se ha registrado y guardado correctamente.");
}

void PurchasesWidget::onSearchChanged() {
    QString cleanQuery = removeAccents(m_searchEdit->text()).trimmed();
    QStringList terms = cleanQuery.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    QVector<PurchaseRecord> filtered;
    for (const auto& p : m_currentPurchases) {
        if (terms.isEmpty()) {
            filtered.append(p);
            continue;
        }
        QString fullText = removeAccents(p.proveedor + " " + p.cifNif + " " + p.numFacturaProveedor + " " + p.concepto + " " + p.fecha.toString("dd/MM/yyyy"));
        bool allMatch = true;
        for (const auto& t : terms) {
            if (!fullText.contains(t)) {
                allMatch = false;
                break;
            }
        }
        if (allMatch) filtered.append(p);
    }

    populateTable(filtered);
}

void PurchasesWidget::populateTable(const QVector<PurchaseRecord>& purchases) {
    m_table->setRowCount(purchases.size());
    m_lblCount->setText(QString("%1 compras registradas").arg(purchases.size()));

    for (int r = 0; r < purchases.size(); ++r) {
        const auto& p = purchases[r];

        // Fecha
        auto* dateItem = new QTableWidgetItem(p.fecha.toString("dd/MM/yyyy"));
        dateItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, 0, dateItem);

        // Proveedor
        m_table->setItem(r, 1, new QTableWidgetItem(p.proveedor));

        // CIF/NIF
        auto* cifItem = new QTableWidgetItem(p.cifNif.isEmpty() ? "-" : p.cifNif);
        cifItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, 2, cifItem);

        // Nº Factura Prov
        auto* numItem = new QTableWidgetItem(p.numFacturaProveedor.isEmpty() ? "-" : p.numFacturaProveedor);
        numItem->setTextAlignment(Qt::AlignCenter);
        numItem->setFont(QFont("Arial", 9, QFont::Bold));
        m_table->setItem(r, 3, numItem);

        // Concepto
        m_table->setItem(r, 4, new QTableWidgetItem(p.concepto));

        // Unidades
        auto* udsItem = new QTableWidgetItem(QString::number(p.unidades, 'f', 0));
        udsItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, 5, udsItem);

        // Precio Unit.
        auto* pUnitItem = new QTableWidgetItem(QString("%1 €").arg(p.precioUnitario, 0, 'f', 2));
        pUnitItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 6, pUnitItem);

        // Base
        auto* baseItem = new QTableWidgetItem(QString("%1 €").arg(p.calcularBase(), 0, 'f', 2));
        baseItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 7, baseItem);

        // IVA
        auto* ivaItem = new QTableWidgetItem(QString("%1 €").arg(p.calcularIva(), 0, 'f', 2));
        ivaItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 8, ivaItem);

        // Total
        auto* totalItem = new QTableWidgetItem(QString("%1 €").arg(p.calcularTotal(), 0, 'f', 2));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totalItem->setFont(QFont("Arial", 9, QFont::Bold));
        m_table->setItem(r, 9, totalItem);

        m_table->setRowHeight(r, 38);
    }
}

void PurchasesWidget::onDeletePurchaseClicked() {}

void PurchasesWidget::onExportRegisterClicked() {
    QString defaultPath = "registro_compras_ventas.xlsx";
    QString fileName = QFileDialog::getSaveFileName(this, "Guardar Libro de Registro de Ventas y Compras", defaultPath, "Archivos de Excel (*.xlsx)");
    if (!fileName.isEmpty()) {
        if (TransactionService::instance().exportToExcel(fileName)) {
            QMessageBox::information(this, "Exportación Exitosa", QString("El libro de registro general se ha guardado correctamente en:\n%1").arg(fileName));
        } else {
            QMessageBox::warning(this, "Error de Exportación", "No se pudo guardar el archivo de Excel.");
        }
    }
}
