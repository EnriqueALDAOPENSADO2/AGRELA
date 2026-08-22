#include "CatalogWidget.h"
#include "../services/CatalogService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPixmap>
#include <QFile>
#include <QMessageBox>

CatalogWidget::CatalogWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
    refreshCategoryCombo();
    onSearchChanged();
}

void CatalogWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // 1. Selector de Categoría / Sección con botón de actualización en caliente
    auto* catLayout = new QHBoxLayout();
    auto* lblCat = new QLabel("Categoría:", this);
    lblCat->setStyleSheet("font-weight: 600; color: #1E293B; font-size: 12px;");

    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->setStyleSheet(
        "QComboBox { background-color: #FFFFFF; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 5px 8px; font-size: 13px; font-weight: 500; }"
        "QComboBox:focus { border: 1.5px solid #2B78C5; }"
        "QComboBox QAbstractItemView { background-color: #FFFFFF; color: #1E293B; selection-background-color: #D9E1F2; selection-color: #1F4E78; border: 1px solid #CBD5E1; }"
    );

    m_btnRefresh = new QPushButton("🔄 Actualizar", this);
    m_btnRefresh->setCursor(Qt::PointingHandCursor);
    m_btnRefresh->setToolTip("Recargar y sincronizar precios directamente desde los archivos Excel de la carpeta PRECIOS");
    m_btnRefresh->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #1F4E78; border: 1px solid #CBD5E1; border-radius: 6px; padding: 5px 10px; font-weight: 600; font-size: 11px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
        "QPushButton:pressed { background-color: #CBD5E1; }"
    );

    catLayout->addWidget(lblCat);
    catLayout->addWidget(m_categoryCombo, 1);
    catLayout->addWidget(m_btnRefresh);
    layout->addLayout(catLayout);

    // 2. Buscador por texto (insensible a acentos)
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("🔍 Buscar artículo (ej: cajon, guia, motor)...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 7px 10px; font-size: 13px; }"
        "QLineEdit:focus { border: 1.5px solid #2B78C5; }"
    );
    layout->addWidget(m_searchEdit);

    auto* countLayout = new QHBoxLayout();
    m_lblCount = new QLabel("Mostrando 0 artículos", this);
    m_lblCount->setStyleSheet("font-size: 11px; color: #64748B; font-weight: 500;");
    countLayout->addWidget(m_lblCount);
    countLayout->addStretch();
    layout->addLayout(countLayout);

    // 3. Tabla del catálogo
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"Foto", "Código", "Descripción", "PVP (€)", "T-1 (€)", "Unidad"});
    
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->setColumnWidth(0, 52);
    
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setIconSize(QSize(42, 42));
    
    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 6px; gridline-color: #E2E8F0; }"
        "QHeaderView::section { background-color: #1F4E78; color: #FFFFFF; padding: 7px 6px; font-weight: bold; font-size: 12px; border: none; border-right: 1px solid #2B5B84; }"
        "QTableWidget::item { padding: 4px; color: #1E293B; }"
        "QTableWidget::item:selected { background-color: #D9E1F2; color: #1F4E78; font-weight: bold; }"
    );

    layout->addWidget(m_table, 1);

    // 4. Botón Añadir
    m_btnAdd = new QPushButton("➕ Añadir Artículo Seleccionado a la Factura", this);
    m_btnAdd->setCursor(Qt::PointingHandCursor);
    m_btnAdd->setStyleSheet(
        "QPushButton { background-color: #2B78C5; color: #FFFFFF; font-weight: bold; font-size: 13px; padding: 9px; border-radius: 6px; border: none; }"
        "QPushButton:hover { background-color: #1F5F9F; }"
        "QPushButton:pressed { background-color: #164370; }"
    );
    layout->addWidget(m_btnAdd);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &CatalogWidget::onSearchChanged);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CatalogWidget::onCategoryChanged);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &CatalogWidget::onRowDoubleClicked);
    connect(m_btnAdd, &QPushButton::clicked, this, &CatalogWidget::onAddClicked);
    connect(m_btnRefresh, &QPushButton::clicked, this, &CatalogWidget::onRefreshPreciosClicked);
}

void CatalogWidget::refreshCategoryCombo() {
    m_categoryCombo->blockSignals(true);
    QString current = m_categoryCombo->currentText();
    m_categoryCombo->clear();
    m_categoryCombo->addItems(CatalogService::instance().getSheets());
    int idx = m_categoryCombo->findText(current);
    if (idx >= 0) {
        m_categoryCombo->setCurrentIndex(idx);
    } else {
        m_categoryCombo->setCurrentIndex(0);
    }
    m_categoryCombo->blockSignals(false);
}

void CatalogWidget::onRefreshPreciosClicked() {
    CatalogService::instance().syncWithPreciosFolder("PRECIOS", "catalog.json", true);
    refreshCategoryCombo();
    onSearchChanged();
    QMessageBox::information(this, "Tarifas Actualizadas", 
                             "Se han recargado todos los artículos y precios (PVP y T-1) directamente desde los archivos Excel de la carpeta PRECIOS.");
}

void CatalogWidget::onSearchChanged() {
    QString query = m_searchEdit->text();
    QString sheet = m_categoryCombo->currentText();
    m_currentItems = CatalogService::instance().search(query, sheet);
    populateTable(m_currentItems);
}

void CatalogWidget::onCategoryChanged(int) {
    onSearchChanged();
}

void CatalogWidget::populateTable(const QVector<CatalogItem>& items) {
    m_table->setRowCount(items.size());
    m_lblCount->setText(QString("Mostrando %1 artículos").arg(items.size()));

    for (int r = 0; r < items.size(); ++r) {
        const auto& it = items[r];

        // Foto / Croquis
        auto* imgItem = new QTableWidgetItem();
        if (!it.imgPath.isEmpty() && QFile::exists(it.imgPath)) {
            QPixmap pix(it.imgPath);
            imgItem->setIcon(QIcon(pix.scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
        imgItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, 0, imgItem);

        // Código
        auto* codeItem = new QTableWidgetItem(it.code.isEmpty() ? "-" : it.code);
        codeItem->setTextAlignment(Qt::AlignCenter);
        codeItem->setFont(QFont("Arial", 9, QFont::Bold));
        m_table->setItem(r, 1, codeItem);

        // Descripción
        auto* descItem = new QTableWidgetItem(it.desc);
        descItem->setToolTip(QString("<b>%1</b><br>Categoría: %2<br>Hoja: %3").arg(it.desc, it.category, it.sheet));
        m_table->setItem(r, 2, descItem);

        // Precio PVP
        double pvpVal = (it.pvp > 0.0) ? it.pvp : it.p1;
        auto* pvpItem = new QTableWidgetItem(pvpVal > 0 ? QString("%1 €").arg(pvpVal, 0, 'f', 2) : "-");
        pvpItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        pvpItem->setFont(QFont("Arial", 9, QFont::Bold));
        pvpItem->setForeground(QColor("#1F4E78"));
        m_table->setItem(r, 3, pvpItem);

        // Precio T-1
        double t1Val = (it.t1 > 0.0) ? it.t1 : (it.p_t1 > 0.0 ? it.p_t1 : pvpVal);
        auto* t1Item = new QTableWidgetItem(t1Val > 0 ? QString("%1 €").arg(t1Val, 0, 'f', 2) : "-");
        t1Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        t1Item->setFont(QFont("Arial", 9, QFont::Bold));
        t1Item->setForeground(QColor("#166534"));
        m_table->setItem(r, 4, t1Item);

        // Unidad
        auto* unitItem = new QTableWidgetItem(it.u1.isEmpty() ? "ud." : it.u1);
        unitItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, 5, unitItem);

        m_table->setRowHeight(r, 46);
    }
}

void CatalogWidget::onRowDoubleClicked(int row, int) {
    if (row >= 0 && row < m_currentItems.size()) {
        emit itemSelected(m_currentItems[row]);
    }
}

void CatalogWidget::onAddClicked() {
    int row = m_table->currentRow();
    if (row >= 0 && row < m_currentItems.size()) {
        emit itemSelected(m_currentItems[row]);
    } else {
        QMessageBox::information(this, "Aviso", "Por favor, selecciona un artículo de la tabla del catálogo.");
    }
}
