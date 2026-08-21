#include "SalesWidget.h"
#include "../services/TransactionService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>

// Helper to remove accents
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

SalesWidget::SalesWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
    refreshSales();
}

void SalesWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // 1. Tarjetas de Resumen Financiero de Ventas
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

    auto b1 = makeSummaryBox("Base Imponible Total", "0.00 €", "#1E293B");
    auto b2 = makeSummaryBox("Total IVA Repercutido", "0.00 €", "#D97706");
    auto b3 = makeSummaryBox("Total Ventas Facturadas", "0.00 €", "#166534");

    m_lblTotalBruto = b1.second;
    m_lblTotalIva = b2.second;
    m_lblTotalFacturado = b3.second;

    summaryLayout->addLayout(b1.first);
    summaryLayout->setStretchFactor(b1.first, 1);
    summaryLayout->addLayout(b2.first);
    summaryLayout->setStretchFactor(b2.first, 1);
    summaryLayout->addLayout(b3.first);
    summaryLayout->setStretchFactor(b3.first, 1);

    m_btnExportExcel = new QPushButton("📊 Exportar Libro de Registro (Excel)", this);
    m_btnExportExcel->setCursor(Qt::PointingHandCursor);
    m_btnExportExcel->setStyleSheet(
        "QPushButton { background-color: #10B981; color: #FFFFFF; font-weight: bold; font-size: 12px; padding: 8px 14px; border-radius: 6px; border: none; }"
        "QPushButton:hover { background-color: #059669; }"
    );
    summaryLayout->addWidget(m_btnExportExcel);

    mainLayout->addWidget(summaryCard);

    // 2. Buscador y Filtros
    auto* filterLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("🔍 Buscar venta (cliente, CIF, nº factura, fecha)...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 7px 10px; font-size: 13px; }"
        "QLineEdit:focus { border: 1.5px solid #2B78C5; }"
    );
    filterLayout->addWidget(m_searchEdit, 1);

    m_lblCount = new QLabel("0 ventas registradas", this);
    m_lblCount->setStyleSheet("font-size: 12px; color: #64748B; font-weight: 500;");
    filterLayout->addWidget(m_lblCount);

    mainLayout->addLayout(filterLayout);

    // 3. Tabla de Registro de Ventas
    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        "Fecha", "Nº Factura", "Cliente / Razon Social", "CIF / NIF",
        "Base Imp. (€)", "IVA (€)", "Total (€)", "Acciones"
    });

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);

    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 6px; gridline-color: #E2E8F0; }"
        "QHeaderView::section { background-color: #1F4E78; color: #FFFFFF; padding: 7px 6px; font-weight: bold; font-size: 12px; border: none; border-right: 1px solid #2B5B84; }"
        "QTableWidget::item { padding: 4px; color: #1E293B; }"
        "QTableWidget::item:selected { background-color: #D9E1F2; color: #1F4E78; font-weight: bold; }"
    );

    mainLayout->addWidget(m_table, 1);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &SalesWidget::onSearchChanged);
    connect(m_btnExportExcel, &QPushButton::clicked, this, &SalesWidget::onExportRegisterClicked);
}

void SalesWidget::refreshSales() {
    m_currentSales = TransactionService::instance().getSales();
    
    m_lblTotalBruto->setText(QString("%1 €").arg(TransactionService::instance().getTotalVentasBruto(), 0, 'f', 2));
    m_lblTotalIva->setText(QString("%1 €").arg(TransactionService::instance().getTotalVentasIva(), 0, 'f', 2));
    m_lblTotalFacturado->setText(QString("%1 €").arg(TransactionService::instance().getTotalVentasFacturado(), 0, 'f', 2));

    onSearchChanged();
}

void SalesWidget::onSearchChanged() {
    QString cleanQuery = removeAccents(m_searchEdit->text()).trimmed();
    QStringList terms = cleanQuery.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    QVector<SaleRecord> filtered;
    for (const auto& s : m_currentSales) {
        if (terms.isEmpty()) {
            filtered.append(s);
            continue;
        }
        QString fullText = removeAccents(s.numeroFactura + " " + s.fecha.toString("dd/MM/yyyy") + " " + s.cliente.nombre + " " + s.cliente.cifNif);
        bool allMatch = true;
        for (const auto& t : terms) {
            if (!fullText.contains(t)) {
                allMatch = false;
                break;
            }
        }
        if (allMatch) filtered.append(s);
    }

    populateTable(filtered);
}

void SalesWidget::populateTable(const QVector<SaleRecord>& sales) {
    m_table->setRowCount(sales.size());
    m_lblCount->setText(QString("%1 ventas registradas").arg(sales.size()));

    for (int r = 0; r < sales.size(); ++r) {
        const auto& s = sales[r];

        // Fecha
        auto* dateItem = new QTableWidgetItem(s.fecha.toString("dd/MM/yyyy"));
        dateItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, 0, dateItem);

        // Nº Factura
        auto* numItem = new QTableWidgetItem(s.numeroFactura);
        numItem->setTextAlignment(Qt::AlignCenter);
        numItem->setFont(QFont("Arial", 9, QFont::Bold));
        m_table->setItem(r, 1, numItem);

        // Cliente
        m_table->setItem(r, 2, new QTableWidgetItem(s.cliente.nombre));

        // CIF/NIF
        auto* cifItem = new QTableWidgetItem(s.cliente.cifNif);
        cifItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, 3, cifItem);

        // Base Imponible
        auto* baseItem = new QTableWidgetItem(QString("%1 €").arg(s.baseImponible, 0, 'f', 2));
        baseItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 4, baseItem);

        // IVA
        auto* ivaItem = new QTableWidgetItem(QString("%1 €").arg(s.cuotaIva, 0, 'f', 2));
        ivaItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 5, ivaItem);

        // Total
        auto* totalItem = new QTableWidgetItem(QString("%1 €").arg(s.totalFactura, 0, 'f', 2));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totalItem->setFont(QFont("Arial", 9, QFont::Bold));
        m_table->setItem(r, 6, totalItem);

        // Botones de Acción (Abrir Excel, Abrir PDF, Eliminar)
        auto* btnWidget = new QWidget(this);
        auto* btnLayout = new QHBoxLayout(btnWidget);
        btnLayout->setContentsMargins(2, 2, 2, 2);
        btnLayout->setSpacing(4);

        if (!s.excelPath.isEmpty() && QFile::exists(s.excelPath)) {
            auto* btnXls = new QPushButton("📊 Excel", this);
            btnXls->setToolTip("Abrir factura Excel");
            btnXls->setCursor(Qt::PointingHandCursor);
            btnXls->setStyleSheet("QPushButton { background: #EBF5FB; color: #1F4E78; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; }");
            connect(btnXls, &QPushButton::clicked, [s]() {
                QDesktopServices::openUrl(QUrl::fromLocalFile(s.excelPath));
            });
            btnLayout->addWidget(btnXls);
        }

        if (!s.pdfPath.isEmpty() && QFile::exists(s.pdfPath)) {
            auto* btnPdf = new QPushButton("📑 PDF", this);
            btnPdf->setToolTip("Abrir factura PDF");
            btnPdf->setCursor(Qt::PointingHandCursor);
            btnPdf->setStyleSheet("QPushButton { background: #FEF2F2; color: #991B1B; border: 1px solid #FECACA; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold; }");
            connect(btnPdf, &QPushButton::clicked, [s]() {
                QDesktopServices::openUrl(QUrl::fromLocalFile(s.pdfPath));
            });
            btnLayout->addWidget(btnPdf);
        }

        auto* btnDel = new QPushButton("🗑️", this);
        btnDel->setToolTip("Borrar registro de venta");
        btnDel->setCursor(Qt::PointingHandCursor);
        btnDel->setStyleSheet("QPushButton { background: #F3F4F6; color: #DC2626; border: 1px solid #D1D5DB; border-radius: 4px; padding: 2px 6px; font-size: 11px; }");
        connect(btnDel, &QPushButton::clicked, [this, s]() {
            if (QMessageBox::question(this, "Eliminar Venta", QString("¿Seguro que deseas borrar el registro de venta de la Factura Nº %1?").arg(s.numeroFactura)) == QMessageBox::Yes) {
                TransactionService::instance().removeSale(s.id);
                refreshSales();
            }
        });
        btnLayout->addWidget(btnDel);

        m_table->setCellWidget(r, 7, btnWidget);
        m_table->setRowHeight(r, 40);
    }
}

void SalesWidget::onOpenExcelClicked() {}
void SalesWidget::onOpenPdfClicked() {}
void SalesWidget::onDeleteSaleClicked() {}

void SalesWidget::onExportRegisterClicked() {
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
