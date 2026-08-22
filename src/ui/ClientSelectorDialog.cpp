#include "ClientSelectorDialog.h"
#include "../services/ClientService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

ClientSelectorDialog::ClientSelectorDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Directorio de Clientes - CARPETA CLIENTES");
    resize(980, 580);
    setStyleSheet("background-color: #F8FAFC; color: #1E293B;");

    setupUi();
    onSearchChanged();
}

void ClientSelectorDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    // Cabecera y búsqueda
    auto* topLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("🔍 Buscar cliente por nombre, alias (ej: vencoris), CIF, calle o población...");
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(
        "QLineEdit { background-color: #FFFFFF; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 8px 12px; font-size: 13px; }"
        "QLineEdit:focus { border: 1.5px solid #2B78C5; }"
    );

    m_btnRefresh = new QPushButton("🔄 Actualizar Carpeta", this);
    m_btnRefresh->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #334155; border: 1px solid #CBD5E1; border-radius: 6px; padding: 8px 14px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );

    topLayout->addWidget(m_searchEdit, 1);
    topLayout->addWidget(m_btnRefresh);
    layout->addLayout(topLayout);

    m_lblCount = new QLabel(this);
    m_lblCount->setStyleSheet("font-size: 11px; color: #64748B; font-weight: 500;");
    layout->addWidget(m_lblCount);

    // Tabla de Clientes con 6 columnas
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"Alias / Archivo", "Nombre / Razón Social", "CIF / NIF", "Dirección", "Población", "Provincia"});

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);

    m_table->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 6px; gridline-color: #E2E8F0; }"
        "QHeaderView::section { background-color: #1F4E78; color: #FFFFFF; padding: 8px 6px; font-weight: bold; font-size: 12px; border: none; border-right: 1px solid #2B5B84; }"
        "QTableWidget::item { padding: 6px; color: #1E293B; }"
        "QTableWidget::item:selected { background-color: #D9E1F2; color: #1F4E78; font-weight: bold; }"
    );

    layout->addWidget(m_table, 1);

    // Botones de acción inferiores
    auto* bottomLayout = new QHBoxLayout();
    auto* btnCancel = new QPushButton("Cancelar", this);
    btnCancel->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #334155; border: 1px solid #CBD5E1; border-radius: 6px; padding: 8px 16px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );

    m_btnSelect = new QPushButton("✔️ Cargar Cliente Seleccionado en la Factura", this);
    m_btnSelect->setStyleSheet(
        "QPushButton { background-color: #27AE60; color: #FFFFFF; font-weight: bold; font-size: 13px; padding: 8px 18px; border-radius: 6px; border: none; }"
        "QPushButton:hover { background-color: #219653; }"
        "QPushButton:pressed { background-color: #1E8449; }"
    );

    bottomLayout->addWidget(btnCancel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_btnSelect);
    layout->addLayout(bottomLayout);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &ClientSelectorDialog::onSearchChanged);
    connect(m_btnRefresh, &QPushButton::clicked, this, &ClientSelectorDialog::onRefreshFromFolderClicked);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &ClientSelectorDialog::onRowDoubleClicked);
    connect(m_btnSelect, &QPushButton::clicked, this, &ClientSelectorDialog::onSelectClicked);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void ClientSelectorDialog::onSearchChanged() {
    QString query = m_searchEdit->text();
    m_currentList = ClientService::instance().search(query);
    populateTable(m_currentList);
}

void ClientSelectorDialog::populateTable(const QVector<Customer>& clients) {
    m_table->setRowCount(clients.size());
    m_lblCount->setText(QString("Mostrando %1 clientes de la CARPETA CLIENTES").arg(clients.size()));

    for (int r = 0; r < clients.size(); ++r) {
        const auto& c = clients[r];

        // Alias / Archivo
        auto* aliasItem = new QTableWidgetItem(c.alias.isEmpty() ? "-" : c.alias);
        aliasItem->setFont(QFont("Arial", 9, QFont::Bold));
        aliasItem->setForeground(QBrush(QColor("#1F4E78")));
        m_table->setItem(r, 0, aliasItem);

        // Nombre / Razón Social
        auto* nameItem = new QTableWidgetItem(c.nombre);
        nameItem->setFont(QFont("Arial", 9, QFont::Bold));
        m_table->setItem(r, 1, nameItem);

        // CIF / NIF
        auto* cifItem = new QTableWidgetItem(c.cifNif.isEmpty() ? "-" : c.cifNif);
        cifItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(r, 2, cifItem);

        // Dirección, Población, Provincia
        m_table->setItem(r, 3, new QTableWidgetItem(c.direccion));
        m_table->setItem(r, 4, new QTableWidgetItem(c.poblacion));
        m_table->setItem(r, 5, new QTableWidgetItem(c.provincia));

        m_table->setRowHeight(r, 36);
    }
}

void ClientSelectorDialog::onRowDoubleClicked(int row, int) {
    if (row >= 0 && row < m_currentList.size()) {
        m_selectedClient = m_currentList[row];
        accept();
    }
}

void ClientSelectorDialog::onSelectClicked() {
    int row = m_table->currentRow();
    if (row >= 0 && row < m_currentList.size()) {
        m_selectedClient = m_currentList[row];
        accept();
    } else {
        QMessageBox::information(this, "Aviso", "Por favor, selecciona un cliente de la lista.");
    }
}

void ClientSelectorDialog::onRefreshFromFolderClicked() {
    bool ok = ClientService::instance().syncFolder("CARPETA CLIENTES", "clientes.json");
    onSearchChanged();
    int count = ClientService::instance().getAllClients().size();
    if (ok && count > 0) {
        QMessageBox::information(this, "Clientes Actualizados", 
            QString("Se ha actualizado el listado con éxito desde la carpeta de clientes.\nTotal clientes disponibles: %1").arg(count));
    } else {
        QMessageBox::warning(this, "Aviso", "No se encontraron clientes o la carpeta está vacía.");
    }
}

Customer ClientSelectorDialog::getSelectedClient() const {
    return m_selectedClient;
}
