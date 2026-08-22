#include "MainWindow.h"
#include "LineItemDialog.h"
#include "ClientSelectorDialog.h"
#include "../services/InvoiceGeneratorService.h"
#include "../services/ClientService.h"
#include "../services/CatalogService.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QFile>
#include <QDesktopServices>
#include <QUrl>
#include <QPixmap>
#include <QDoubleSpinBox>
#include <QStringListModel>
#include <QDebug>
#include <QDateTime>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Persianas A Grela - Facturación");
    setWindowIcon(QIcon(":/app_icon.png"));
    if (windowIcon().isNull()) setWindowIcon(QIcon("app_icon.png"));
    if (windowIcon().isNull()) setWindowIcon(QIcon("logo.jpg"));

    resize(1400, 880);
    setMinimumSize(1100, 700);

    ClientService::instance().loadClients("clientes.json", "CARPETA CLIENTES");

    setupUi();
    updateClientCompleter();
    onNewInvoice();
}

void MainWindow::setupUi() {
    auto* centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #F1F5F9;");
    setCentralWidget(centralWidget);

    auto* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(12, 10, 12, 10);
    rootLayout->setSpacing(10);

    // 1. Cabecera Corporativa con Logo
    setupHeader();
    rootLayout->addWidget(findChild<QFrame*>("headerFrame"));

    // 2. Panel Dividido (Splitter): Catálogo a la Izquierda | Factura a la Derecha
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStyleSheet(
        "QSplitter::handle { background-color: #CBD5E1; width: 4px; }"
        "QSplitter::handle:hover { background-color: #2B78C5; }"
    );

    // Panel Izquierdo: Catálogo de Productos
    auto* leftContainer = new QGroupBox("Catálogo de Productos - Persianas A Grela", this);
    leftContainer->setStyleSheet(
        "QGroupBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; margin-top: 10px; font-weight: bold; color: #1F4E78; font-size: 13px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 14px; padding: 0 6px; background-color: #FFFFFF; }"
    );
    auto* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(8, 14, 8, 8);
    m_catalogWidget = new CatalogWidget(this);
    leftLayout->addWidget(m_catalogWidget);
    splitter->addWidget(leftContainer);

    // Panel Derecho: Factura en edición
    auto* rightContainer = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);

    // Bloque 1: Datos Cliente y Factura
    setupClientAndInvoiceMeta();
    rightLayout->addWidget(findChild<QGroupBox*>("metaGroup"));

    // Bloque 2: Tabla de Líneas
    setupLinesTable();
    rightLayout->addWidget(findChild<QGroupBox*>("linesGroup"), 1);

    // Bloque 3: Resumen y Acciones
    auto* bottomLayout = new QHBoxLayout();
    setupTotalsCard();
    setupActionsBar();

    bottomLayout->addWidget(findChild<QFrame*>("actionsFrame"), 3);
    bottomLayout->addWidget(findChild<QFrame*>("totalsCard"), 2);
    rightLayout->addLayout(bottomLayout);

    splitter->addWidget(rightContainer);
    splitter->setSizes({480, 880});
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);

    rootLayout->addWidget(splitter, 1);

    // Conectar selección en catálogo
    connect(m_catalogWidget, &CatalogWidget::itemSelected, this, &MainWindow::onCatalogItemSelected);
}

void MainWindow::setupHeader() {
    auto* headerFrame = new QFrame(this);
    headerFrame->setObjectName("headerFrame");
    headerFrame->setStyleSheet(
        "QFrame#headerFrame { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1F4E78, stop:0.6 #163B5C, stop:1 #1B4B75); border-radius: 8px; border: none; }"
        "QFrame#headerFrame QLabel { background: transparent; background-color: transparent; border: none; }"
    );

    auto* layout = new QHBoxLayout(headerFrame);
    layout->setContentsMargins(14, 9, 16, 9);
    layout->setSpacing(16);

    auto* lblLogo = new QLabel(this);
    lblLogo->setStyleSheet("background-color: #FFFFFF; border-radius: 7px; padding: 4px 6px; border: 1px solid rgba(255,255,255,0.3);");
    QString logoPath = ":/logo.jpg";
    if (!QFile::exists(logoPath)) logoPath = "logo.jpg";
    if (!QFile::exists(logoPath)) logoPath = "logo.jpeg";
    if (QFile::exists(logoPath)) {
        QPixmap pix(logoPath);
        lblLogo->setPixmap(pix.scaled(135, 52, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    layout->addWidget(lblLogo);

    auto* textLayout = new QVBoxLayout();
    textLayout->setSpacing(3);
    textLayout->setContentsMargins(0, 0, 0, 0);

    auto* lblTitle = new QLabel("Persianas A Grela", this);
    lblTitle->setStyleSheet("background: transparent; background-color: transparent; border: none; color: #FFFFFF; font-size: 21px; font-weight: bold; padding: 0; margin: 0;");

    auto* lblSubtitle = new QLabel("Juan Manuel Aldao López   |   C/ Gutemberg Nº 44-A, Polígono A Grela   |   C.P. 15008 A Coruña   |   D.N.I. 52434449-S", this);
    lblSubtitle->setStyleSheet("background: transparent; background-color: transparent; border: none; color: #D9E1F2; font-size: 12px; font-weight: 500; padding: 0; margin: 0;");

    textLayout->addWidget(lblTitle);
    textLayout->addWidget(lblSubtitle);
    layout->addLayout(textLayout);
    layout->addStretch();
}

void MainWindow::setupClientAndInvoiceMeta() {
    auto* metaGroup = new QGroupBox("Datos del Cliente y Factura", this);
    metaGroup->setObjectName("metaGroup");
    metaGroup->setStyleSheet(
        "QGroupBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; margin-top: 10px; font-weight: bold; color: #1F4E78; font-size: 13px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 14px; padding: 0 6px; background-color: #FFFFFF; }"
        "QLabel { color: #1E293B; font-size: 12px; font-weight: 600; }"
        "QLineEdit, QDateEdit, QComboBox { background-color: #FFFFFF; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 5px; padding: 5px 8px; font-size: 12px; }"
        "QLineEdit:focus, QDateEdit:focus, QComboBox:focus { border: 1.5px solid #2B78C5; }"
        "QComboBox QAbstractItemView { background-color: #FFFFFF; color: #1E293B; selection-background-color: #D9E1F2; selection-color: #1F4E78; border: 1px solid #CBD5E1; }"
    );

    auto* layout = new QGridLayout(metaGroup);
    layout->setContentsMargins(12, 16, 12, 12);
    layout->setHorizontalSpacing(10);
    layout->setVerticalSpacing(8);

    m_txtClientNombre = new QLineEdit(this);
    m_txtClientNombre->setPlaceholderText("Escribe para autocompletar o selecciona...");
    
    auto* btnBrowseClients = new QPushButton("👥 CARPETA CLIENTES", this);
    btnBrowseClients->setCursor(Qt::PointingHandCursor);
    btnBrowseClients->setStyleSheet(
        "QPushButton { background-color: #EBF5FB; color: #1F4E78; border: 1.5px solid #2B78C5; border-radius: 5px; padding: 5px 10px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background-color: #D9E1F2; }"
    );
    btnBrowseClients->setToolTip("Buscar y rellenar automáticamente desde los clientes de CARPETA CLIENTES");

    m_txtClientCif = new QLineEdit(this);
    m_txtClientCif->setPlaceholderText("CIF / NIF");
    m_txtClientDireccion = new QLineEdit(this);
    m_txtClientDireccion->setPlaceholderText("Dirección completa");
    m_txtClientPoblacion = new QLineEdit(this);
    m_txtClientPoblacion->setPlaceholderText("Población / C.P.");
    m_txtClientProvincia = new QLineEdit(this);
    m_txtClientProvincia->setPlaceholderText("Provincia");

    auto* clientNombreLayout = new QHBoxLayout();
    clientNombreLayout->addWidget(m_txtClientNombre, 1);
    clientNombreLayout->addWidget(btnBrowseClients);

    m_txtNumFactura = new QLineEdit(this);
    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat("dd/MM/yyyy");

    m_dateVencimiento = new QDateEdit(QDate::currentDate(), this);
    m_dateVencimiento->setCalendarPopup(true);
    m_dateVencimiento->setDisplayFormat("dd/MM/yyyy");
    m_dateVencimiento->setStyleSheet("background-color: #FEF3C7; color: #92400E; font-weight: bold; border: 1.5px solid #F59E0B; border-radius: 5px; padding: 4px 6px;");

    m_cmbFormaPago = new QComboBox(this);
    m_cmbFormaPago->addItems({"TPV", "Giro bancario 30 días", "Giro bancario 60 días", "Transferencia bancaria", "Efectivo", "Pagaré"});

    m_cmbTarifa = new QComboBox(this);
    m_cmbTarifa->addItem("PVP (Tarifa General)", "PVP");
    m_cmbTarifa->addItem("Tarifa 1 (T-1 Distribuidor)", "T1");
    m_cmbTarifa->setStyleSheet("font-weight: bold; color: #1F4E78; background-color: #F8FAFC; border: 1.5px solid #2B78C5; border-radius: 5px; padding: 4px 6px;");

    layout->addWidget(new QLabel("Cliente:", this), 0, 0);
    layout->addLayout(clientNombreLayout, 0, 1);
    layout->addWidget(new QLabel("CIF/NIF:", this), 0, 2);
    layout->addWidget(m_txtClientCif, 0, 3);

    layout->addWidget(new QLabel("Dirección:", this), 1, 0);
    layout->addWidget(m_txtClientDireccion, 1, 1);
    layout->addWidget(new QLabel("Población:", this), 1, 2);
    layout->addWidget(m_txtClientPoblacion, 1, 3);

    layout->addWidget(new QLabel("Provincia:", this), 2, 0);
    layout->addWidget(m_txtClientProvincia, 2, 1);

    auto* invoiceRow1 = new QHBoxLayout();
    invoiceRow1->addWidget(new QLabel("Nº:", this));
    invoiceRow1->addWidget(m_txtNumFactura);
    invoiceRow1->addWidget(new QLabel("Tarifa:", this));
    invoiceRow1->addWidget(m_cmbTarifa);
    layout->addLayout(invoiceRow1, 2, 2, 1, 2);

    auto* invoiceRow2 = new QHBoxLayout();
    invoiceRow2->addWidget(new QLabel("Fecha Emisión:", this));
    invoiceRow2->addWidget(m_dateEdit);
    invoiceRow2->addWidget(new QLabel("Pago:", this));
    invoiceRow2->addWidget(m_cmbFormaPago);
    invoiceRow2->addWidget(new QLabel("Vencimiento:", this));
    invoiceRow2->addWidget(m_dateVencimiento);
    layout->addLayout(invoiceRow2, 3, 0, 1, 4);

    connect(btnBrowseClients, &QPushButton::clicked, this, &MainWindow::onBrowseClientsClicked);
    connect(m_cmbTarifa, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onTarifaChanged);
    connect(m_cmbFormaPago, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &MainWindow::onFormaPagoChanged);
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &MainWindow::onFechaEmisionChanged);
}

void MainWindow::updateClientCompleter() {
    const auto& clients = ClientService::instance().getAllClients();
    QStringList names;
    for (const auto& c : clients) {
        if (!c.nombre.isEmpty()) names << c.nombre;
        if (!c.alias.isEmpty() && c.alias != c.nombre) {
            names << c.alias;
            names << QString("%1 (%2)").arg(c.alias, c.nombre);
        }
    }
    names.removeDuplicates();

    if (!m_clientCompleter) {
        m_clientCompleter = new QCompleter(names, this);
        m_clientCompleter->setCaseSensitivity(Qt::CaseInsensitive);
        m_clientCompleter->setFilterMode(Qt::MatchContains);
        m_txtClientNombre->setCompleter(m_clientCompleter);
        connect(m_clientCompleter, QOverload<const QString&>::of(&QCompleter::activated), this, &MainWindow::onClientCompleterActivated);
    } else {
        auto* model = new QStringListModel(names, m_clientCompleter);
        m_clientCompleter->setModel(model);
    }
}

void MainWindow::onBrowseClientsClicked() {
    ClientSelectorDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        applyCustomerData(dlg.getSelectedClient());
    }
}

void MainWindow::onClientCompleterActivated(const QString& name) {
    Customer c = ClientService::instance().findByName(name);
    if (!c.nombre.isEmpty()) {
        applyCustomerData(c);
    }
}

void MainWindow::applyCustomerData(const Customer& c) {
    m_txtClientNombre->setText(c.nombre);
    m_txtClientCif->setText(c.cifNif);
    m_txtClientDireccion->setText(c.direccion);
    m_txtClientPoblacion->setText(c.poblacion);
    m_txtClientProvincia->setText(c.provincia);
}

void MainWindow::setupLinesTable() {
    auto* linesGroup = new QGroupBox("Líneas de la Factura", this);
    linesGroup->setObjectName("linesGroup");
    linesGroup->setStyleSheet(
        "QGroupBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; margin-top: 10px; font-weight: bold; color: #1F4E78; font-size: 13px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 14px; padding: 0 6px; background-color: #FFFFFF; }"
    );

    auto* layout = new QVBoxLayout(linesGroup);
    layout->setContentsMargins(10, 16, 10, 10);
    layout->setSpacing(8);

    m_tableLines = new QTableWidget(this);
    m_tableLines->setColumnCount(9);
    m_tableLines->setHorizontalHeaderLabels({
        "Foto", "Descripción", "Uds.", "Precio Unit. (€)", 
        "Ancho Persiana (mm)", "Ancho Rollo (mm)", "Alto (mm)", "M² Total", "Total (€)"
    });

    m_tableLines->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_tableLines->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tableLines->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tableLines->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tableLines->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_tableLines->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_tableLines->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_tableLines->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_tableLines->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_tableLines->setColumnWidth(0, 48);

    m_tableLines->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableLines->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableLines->setAlternatingRowColors(true);
    m_tableLines->verticalHeader()->setVisible(true);
    m_tableLines->setIconSize(QSize(36, 36));

    m_tableLines->setStyleSheet(
        "QTableWidget { background-color: #FFFFFF; alternate-background-color: #F8FAFC; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 6px; gridline-color: #E2E8F0; }"
        "QHeaderView::section { background-color: #2B5B84; color: #FFFFFF; padding: 7px 6px; font-weight: bold; font-size: 12px; border: none; border-right: 1px solid #1F4E78; }"
        "QTableWidget::item { padding: 4px; color: #1E293B; }"
        "QTableWidget::item:selected { background-color: #D9E1F2; color: #1F4E78; font-weight: bold; }"
    );

    layout->addWidget(m_tableLines);

    auto* btnLayout = new QHBoxLayout();
    auto* btnAddCustom = new QPushButton("➕ Añadir Línea Libre", this);
    auto* btnEdit = new QPushButton("✏️ Editar Línea", this);
    auto* btnDelete = new QPushButton("🗑️ Eliminar Línea", this);
    auto* btnUp = new QPushButton("⬆️ Subir", this);
    auto* btnDown = new QPushButton("⬇️ Bajar", this);

    QString btnStyle = 
        "QPushButton { background-color: #F1F5F9; color: #334155; border: 1px solid #CBD5E1; border-radius: 6px; padding: 6px 12px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
        "QPushButton:pressed { background-color: #CBD5E1; }";

    btnAddCustom->setStyleSheet(btnStyle);
    btnEdit->setStyleSheet(btnStyle);
    btnDelete->setStyleSheet(btnStyle);
    btnUp->setStyleSheet(btnStyle);
    btnDown->setStyleSheet(btnStyle);

    btnLayout->addWidget(btnAddCustom);
    btnLayout->addWidget(btnEdit);
    btnLayout->addWidget(btnDelete);
    btnLayout->addStretch();
    btnLayout->addWidget(btnUp);
    btnLayout->addWidget(btnDown);
    layout->addLayout(btnLayout);

    connect(btnAddCustom, &QPushButton::clicked, this, &MainWindow::onAddCustomLine);
    connect(btnEdit, &QPushButton::clicked, this, &MainWindow::onEditSelectedLine);
    connect(btnDelete, &QPushButton::clicked, this, &MainWindow::onDeleteSelectedLine);
    connect(btnUp, &QPushButton::clicked, this, &MainWindow::onMoveLineUp);
    connect(btnDown, &QPushButton::clicked, this, &MainWindow::onMoveLineDown);
    connect(m_tableLines, &QTableWidget::cellDoubleClicked, this, &MainWindow::onLineDoubleClicked);
}

void MainWindow::setupTotalsCard() {
    auto* totalsCard = new QFrame(this);
    totalsCard->setObjectName("totalsCard");
    totalsCard->setStyleSheet(
        "QFrame#totalsCard { background-color: #FFFFFF; border: 2px solid #1F4E78; border-radius: 8px; padding: 10px; }"
        "QLabel { color: #1E293B; font-size: 13px; }"
        "QDoubleSpinBox { background-color: #FFFFFF; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 4px; padding: 2px 6px; font-size: 12px; }"
    );

    auto* layout = new QGridLayout(totalsCard);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setHorizontalSpacing(16);
    layout->setVerticalSpacing(6);

    m_lblTotalBruto = new QLabel("0.00 €", this);
    m_lblTotalBruto->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lblTotalBruto->setStyleSheet("font-size: 14px; font-weight: bold; color: #1E293B;");

    m_spnTipoIva = new QDoubleSpinBox(this);
    m_spnTipoIva->setRange(0.0, 100.0);
    m_spnTipoIva->setValue(21.0);
    m_spnTipoIva->setSuffix(" %");
    m_spnTipoIva->setFixedWidth(75);

    m_lblCuotaIva = new QLabel("0.00 €", this);
    m_lblCuotaIva->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lblCuotaIva->setStyleSheet("font-size: 14px; font-weight: bold; color: #1E293B;");

    m_lblTotalFactura = new QLabel("0.00 €", this);
    m_lblTotalFactura->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_lblTotalFactura->setStyleSheet("font-size: 19px; font-weight: bold; color: #1F4E78;");

    auto* lblBrutoText = new QLabel("Total Bruto (Base Imp.):", this);
    auto* lblIvaText = new QLabel("I.V.A.:", this);
    auto* lblTotalText = new QLabel("TOTAL FACTURA:", this);
    lblTotalText->setStyleSheet("font-size: 14px; font-weight: bold; color: #1F4E78;");

    layout->addWidget(lblBrutoText, 0, 0);
    layout->addWidget(m_lblTotalBruto, 0, 2);

    auto* ivaLayout = new QHBoxLayout();
    ivaLayout->addWidget(lblIvaText);
    ivaLayout->addWidget(m_spnTipoIva);
    layout->addLayout(ivaLayout, 1, 0);
    layout->addWidget(m_lblCuotaIva, 1, 2);

    layout->addWidget(lblTotalText, 2, 0);
    layout->addWidget(m_lblTotalFactura, 2, 2);

    connect(m_spnTipoIva, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double){
        updateTotals();
    });
}

void MainWindow::setupActionsBar() {
    auto* actionsFrame = new QFrame(this);
    actionsFrame->setObjectName("actionsFrame");
    actionsFrame->setStyleSheet("QFrame#actionsFrame { background: transparent; border: none; }");

    auto* layout = new QVBoxLayout(actionsFrame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* btnGenerateBoth = new QPushButton("⚡ GENERAR FACTURA (EXCEL + PDF SIMULTÁNEO)", this);
    btnGenerateBoth->setCursor(Qt::PointingHandCursor);
    btnGenerateBoth->setStyleSheet(
        "QPushButton { background-color: #16A34A; color: #FFFFFF; font-weight: bold; font-size: 14px; padding: 11px; border-radius: 6px; border: none; }"
        "QPushButton:hover { background-color: #15803D; }"
        "QPushButton:pressed { background-color: #166534; }"
    );

    auto* subBtnLayout = new QHBoxLayout();
    auto* btnExcel = new QPushButton("📊 Solo Excel", this);
    auto* btnPdf = new QPushButton("📑 Solo PDF", this);
    auto* btnNew = new QPushButton("🆕 Nueva", this);
    auto* btnSave = new QPushButton("💾 Guardar", this);
    auto* btnLoad = new QPushButton("📂 Cargar", this);

    QString subBtnStyle = 
        "QPushButton { background-color: #F1F5F9; color: #334155; border: 1px solid #CBD5E1; border-radius: 5px; padding: 6px 8px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
        "QPushButton:pressed { background-color: #CBD5E1; }";

    btnExcel->setStyleSheet(subBtnStyle);
    btnPdf->setStyleSheet(subBtnStyle);
    btnNew->setStyleSheet(subBtnStyle);
    btnSave->setStyleSheet(subBtnStyle);
    btnLoad->setStyleSheet(subBtnStyle);

    subBtnLayout->addWidget(btnExcel);
    subBtnLayout->addWidget(btnPdf);
    subBtnLayout->addWidget(btnNew);
    subBtnLayout->addWidget(btnSave);
    subBtnLayout->addWidget(btnLoad);

    layout->addWidget(btnGenerateBoth);
    layout->addLayout(subBtnLayout);

    connect(btnGenerateBoth, &QPushButton::clicked, this, &MainWindow::onGenerateBothClicked);
    connect(btnExcel, &QPushButton::clicked, this, &MainWindow::onExportExcelOnly);
    connect(btnPdf, &QPushButton::clicked, this, &MainWindow::onExportPdfOnly);
    connect(btnNew, &QPushButton::clicked, this, &MainWindow::onNewInvoice);
    connect(btnSave, &QPushButton::clicked, this, &MainWindow::onSaveInvoiceJson);
    connect(btnLoad, &QPushButton::clicked, this, &MainWindow::onLoadInvoiceJson);
}

void MainWindow::onTarifaChanged(int) {
    QString newTariff = m_cmbTarifa ? m_cmbTarifa->currentData().toString() : "PVP";
    m_currentInvoice.tarifa = newTariff;

    if (!m_currentInvoice.items.isEmpty()) {
        for (auto& it : m_currentInvoice.items) {
            it.tarifa = newTariff;
            CatalogItem cat = CatalogService::instance().findByCode(it.code);
            if (!cat.code.isEmpty()) {
                it.precioUnitario = cat.getPriceForTariff(newTariff);
            }
        }
        refreshInvoiceTable();
    }
}

void MainWindow::onFormaPagoChanged(const QString& formaPago) {
    if (!m_dateEdit || !m_dateVencimiento) return;
    QDate fEmision = m_dateEdit->date();
    QString fpLower = formaPago.toLower();

    if (fpLower.contains("30")) {
        m_dateVencimiento->setDate(fEmision.addDays(30));
    } else if (fpLower.contains("60")) {
        m_dateVencimiento->setDate(fEmision.addDays(60));
    } else if (fpLower.contains("90")) {
        m_dateVencimiento->setDate(fEmision.addDays(90));
    } else if (fpLower.contains("transferencia")) {
        m_dateVencimiento->setDate(fEmision.addDays(30));
    } else if (fpLower.contains("pagar")) {
        m_dateVencimiento->setDate(fEmision.addDays(60));
    } else {
        // TPV, Efectivo, Contado
        m_dateVencimiento->setDate(fEmision);
    }
}

void MainWindow::onFechaEmisionChanged(const QDate&) {
    if (m_cmbFormaPago) {
        onFormaPagoChanged(m_cmbFormaPago->currentText());
    }
}

void MainWindow::onCatalogItemSelected(const CatalogItem& catItem) {
    QString currentTariff = m_cmbTarifa ? m_cmbTarifa->currentData().toString() : "PVP";
    InvoiceItem invItem = InvoiceItem::fromCatalogItem(catItem, currentTariff);
    LineItemDialog dlg(invItem, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_currentInvoice.items.append(dlg.getItem());
        refreshInvoiceTable();
    }
}

void MainWindow::onAddCustomLine() {
    InvoiceItem emptyItem;
    emptyItem.desc = "Artículo personalizado";
    emptyItem.unidades = 1.0;
    emptyItem.tarifa = m_cmbTarifa ? m_cmbTarifa->currentData().toString() : "PVP";
    emptyItem.precioUnitario = 0.0;
    LineItemDialog dlg(emptyItem, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_currentInvoice.items.append(dlg.getItem());
        refreshInvoiceTable();
    }
}

void MainWindow::onEditSelectedLine() {
    int row = m_tableLines->currentRow();
    if (row >= 0 && row < m_currentInvoice.items.size()) {
        LineItemDialog dlg(m_currentInvoice.items[row], this);
        if (dlg.exec() == QDialog::Accepted) {
            m_currentInvoice.items[row] = dlg.getItem();
            refreshInvoiceTable();
        }
    }
}

void MainWindow::onLineDoubleClicked(int row, int) {
    if (row >= 0 && row < m_currentInvoice.items.size()) {
        LineItemDialog dlg(m_currentInvoice.items[row], this);
        if (dlg.exec() == QDialog::Accepted) {
            m_currentInvoice.items[row] = dlg.getItem();
            refreshInvoiceTable();
        }
    }
}

void MainWindow::onDeleteSelectedLine() {
    int row = m_tableLines->currentRow();
    if (row >= 0 && row < m_currentInvoice.items.size()) {
        m_currentInvoice.items.removeAt(row);
        refreshInvoiceTable();
    }
}

void MainWindow::onMoveLineUp() {
    int row = m_tableLines->currentRow();
    if (row > 0 && row < m_currentInvoice.items.size()) {
        m_currentInvoice.items.swapItemsAt(row, row - 1);
        refreshInvoiceTable();
        m_tableLines->selectRow(row - 1);
    }
}

void MainWindow::onMoveLineDown() {
    int row = m_tableLines->currentRow();
    if (row >= 0 && row < m_currentInvoice.items.size() - 1) {
        m_currentInvoice.items.swapItemsAt(row, row + 1);
        refreshInvoiceTable();
        m_tableLines->selectRow(row + 1);
    }
}

void MainWindow::refreshInvoiceTable() {
    m_tableLines->setRowCount(m_currentInvoice.items.size());
    for (int r = 0; r < m_currentInvoice.items.size(); ++r) {
        const auto& it = m_currentInvoice.items[r];

        auto* imgItem = new QTableWidgetItem();
        if (!it.imgPath.isEmpty() && QFile::exists(it.imgPath)) {
            QPixmap pix(it.imgPath);
            imgItem->setIcon(QIcon(pix.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
        imgItem->setTextAlignment(Qt::AlignCenter);
        m_tableLines->setItem(r, 0, imgItem);

        m_tableLines->setItem(r, 1, new QTableWidgetItem(it.desc));

        auto* udsItem = new QTableWidgetItem(QString::number(it.unidades, 'f', 0));
        udsItem->setTextAlignment(Qt::AlignCenter);
        m_tableLines->setItem(r, 2, udsItem);

        QString tag = it.tarifa.isEmpty() ? "PVP" : it.tarifa;
        auto* priceItem = new QTableWidgetItem(QString("%1 € (%2)").arg(it.precioUnitario, 0, 'f', 2).arg(tag));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tableLines->setItem(r, 3, priceItem);

        auto* anchoFinalItem = new QTableWidgetItem(it.anchoPersianaFinal > 0 ? QString("%1 mm").arg(it.anchoPersianaFinal, 0, 'f', 0) : "-");
        anchoFinalItem->setTextAlignment(Qt::AlignCenter);
        m_tableLines->setItem(r, 4, anchoFinalItem);

        auto* anchoRolloItem = new QTableWidgetItem(it.anchoRolloUsado > 0 ? QString("%1 mm").arg(it.anchoRolloUsado, 0, 'f', 0) : "-");
        anchoRolloItem->setTextAlignment(Qt::AlignCenter);
        m_tableLines->setItem(r, 5, anchoRolloItem);

        auto* altoItem = new QTableWidgetItem(it.alto > 0 ? QString("%1 mm").arg(it.alto, 0, 'f', 0) : "-");
        altoItem->setTextAlignment(Qt::AlignCenter);
        m_tableLines->setItem(r, 6, altoItem);

        double m2 = it.calcularMetrosCuadrados();
        auto* m2Item = new QTableWidgetItem(m2 > 0 ? QString("%1 m²").arg(m2, 0, 'f', 3) : "-");
        m2Item->setTextAlignment(Qt::AlignCenter);
        m_tableLines->setItem(r, 7, m2Item);

        auto* totalItem = new QTableWidgetItem(QString("%1 €").arg(it.calcularTotal(), 0, 'f', 2));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totalItem->setFont(QFont("Arial", 9, QFont::Bold));
        m_tableLines->setItem(r, 8, totalItem);

        m_tableLines->setRowHeight(r, 42);
    }
    updateTotals();
}

void MainWindow::updateTotals() {
    m_currentInvoice.tipoIva = m_spnTipoIva->value() / 100.0;
    double bruto = m_currentInvoice.calcularTotalBruto();
    double cuota = m_currentInvoice.calcularCuotaIva();
    double total = m_currentInvoice.calcularTotalFactura();

    m_lblTotalBruto->setText(QString("%1 €").arg(bruto, 0, 'f', 2));
    m_lblCuotaIva->setText(QString("%1 €").arg(cuota, 0, 'f', 2));
    m_lblTotalFactura->setText(QString("%1 €").arg(total, 0, 'f', 2));
}

Invoice MainWindow::getInvoiceFromUi() const {
    Invoice inv = m_currentInvoice;
    inv.cliente.nombre = m_txtClientNombre->text().trimmed();
    inv.cliente.cifNif = m_txtClientCif->text().trimmed();
    inv.cliente.direccion = m_txtClientDireccion->text().trimmed();
    inv.cliente.poblacion = m_txtClientPoblacion->text().trimmed();
    inv.cliente.provincia = m_txtClientProvincia->text().trimmed();

    inv.numeroFactura = m_txtNumFactura->text().trimmed();
    inv.fecha = m_dateEdit->date();
    inv.fechaVencimiento = m_dateVencimiento ? m_dateVencimiento->date() : inv.fecha;
    inv.formaPago = m_cmbFormaPago->currentText();
    inv.tarifa = m_cmbTarifa ? m_cmbTarifa->currentData().toString() : "PVP";
    inv.tipoIva = m_spnTipoIva->value() / 100.0;
    return inv;
}

void MainWindow::loadInvoiceToUi(const Invoice& inv) {
    m_currentInvoice = inv;
    m_txtClientNombre->setText(inv.cliente.nombre);
    m_txtClientCif->setText(inv.cliente.cifNif);
    m_txtClientDireccion->setText(inv.cliente.direccion);
    m_txtClientPoblacion->setText(inv.cliente.poblacion);
    m_txtClientProvincia->setText(inv.cliente.provincia);

    m_txtNumFactura->setText(inv.numeroFactura);
    m_dateEdit->setDate(inv.fecha);
    if (m_dateVencimiento) {
        m_dateVencimiento->setDate(inv.fechaVencimiento.isValid() ? inv.fechaVencimiento : inv.fecha);
    }
    
    int idx = m_cmbFormaPago->findText(inv.formaPago);
    if (idx >= 0) m_cmbFormaPago->setCurrentIndex(idx);

    if (m_cmbTarifa) {
        int tIdx = m_cmbTarifa->findData(inv.tarifa.isEmpty() ? "PVP" : inv.tarifa);
        if (tIdx >= 0) m_cmbTarifa->setCurrentIndex(tIdx);
    }

    m_spnTipoIva->setValue(inv.tipoIva * 100.0);
    refreshInvoiceTable();
}

void MainWindow::onNewInvoice() {
    Invoice fresh;
    fresh.numeroFactura = QString::number(QDateTime::currentDateTime().toSecsSinceEpoch() % 10000);
    fresh.fecha = QDate::currentDate();
    fresh.fechaVencimiento = fresh.fecha;
    loadInvoiceToUi(fresh);
    if (m_cmbFormaPago) {
        onFormaPagoChanged(m_cmbFormaPago->currentText());
    }
}

void MainWindow::onGenerateBothClicked() {
    Invoice inv = getInvoiceFromUi();
    if (inv.items.isEmpty()) {
        QMessageBox::warning(this, "Aviso", "La factura no contiene ningún artículo.");
        return;
    }

    auto paths = InvoiceGeneratorService::instance().generateBoth(inv, "facturas");
    if (!paths.first.isEmpty() && !paths.second.isEmpty()) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Factura Generada con Éxito");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(
            "QMessageBox { background-color: #FFFFFF; }"
            "QLabel { color: #1E293B; font-size: 13px; }"
            "QPushButton { background-color: #F1F5F9; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 5px; padding: 6px 14px; font-weight: 600; min-width: 90px; }"
            "QPushButton:hover { background-color: #E2E8F0; }"
        );
        msgBox.setText(QString("<h3>¡Factura generada con éxito!</h3>"
                               "<p>Se han creado los siguientes archivos:</p>"
                               "<ul>"
                               "<li><b>Excel:</b> %1</li>"
                               "<li><b>PDF:</b> %2</li>"
                               "</ul>").arg(paths.first, paths.second));

        auto* btnOpenPdf = msgBox.addButton("📑 Abrir PDF", QMessageBox::ActionRole);
        auto* btnOpenExcel = msgBox.addButton("📊 Abrir Excel", QMessageBox::ActionRole);
        auto* btnOpenFolder = msgBox.addButton("📁 Abrir Carpeta", QMessageBox::ActionRole);
        msgBox.addButton(QMessageBox::Close);

        msgBox.exec();

        if (msgBox.clickedButton() == btnOpenPdf) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(paths.second));
        } else if (msgBox.clickedButton() == btnOpenExcel) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(paths.first));
        } else if (msgBox.clickedButton() == btnOpenFolder) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(paths.first).absolutePath()));
        }
    } else {
        QMessageBox::critical(this, "Error", "Ocurrió un problema generando los archivos de la factura.");
    }
}

void MainWindow::onExportExcelOnly() {
    Invoice inv = getInvoiceFromUi();
    if (inv.items.isEmpty()) {
        QMessageBox::warning(this, "Aviso", "La factura no contiene ningún artículo.");
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, "Guardar Factura Excel", QString("facturas/Factura_%1.xlsx").arg(inv.numeroFactura), "Archivos Excel (*.xlsx)");
    if (!path.isEmpty()) {
        if (InvoiceGeneratorService::instance().generateExcel(inv, path)) {
            QMessageBox::information(this, "Éxito", "Factura Excel guardada correctamente.");
        } else {
            QMessageBox::critical(this, "Error", "No se pudo guardar la factura en Excel.");
        }
    }
}

void MainWindow::onExportPdfOnly() {
    Invoice inv = getInvoiceFromUi();
    if (inv.items.isEmpty()) {
        QMessageBox::warning(this, "Aviso", "La factura no contiene ningún artículo.");
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, "Guardar Factura PDF", QString("facturas/Factura_%1.pdf").arg(inv.numeroFactura), "Archivos PDF (*.pdf)");
    if (!path.isEmpty()) {
        if (InvoiceGeneratorService::instance().generatePdf(inv, path)) {
            QMessageBox::information(this, "Éxito", "Factura PDF guardada correctamente.");
        } else {
            QMessageBox::critical(this, "Error", "No se pudo guardar la factura en PDF.");
        }
    }
}

void MainWindow::onSaveInvoiceJson() {
    Invoice inv = getInvoiceFromUi();
    QString path = QFileDialog::getSaveFileName(this, "Guardar Proyecto de Factura", QString("facturas/Factura_%1.agrfac").arg(inv.numeroFactura), "Factura AGRELA (*.agrfac *.json)");
    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(inv.toJson()).toJson());
            file.close();
            QMessageBox::information(this, "Éxito", "Proyecto de factura guardado correctamente.");
        }
    }
}

void MainWindow::onLoadInvoiceJson() {
    QString path = QFileDialog::getOpenFileName(this, "Cargar Proyecto de Factura", "facturas", "Factura AGRELA (*.agrfac *.json)");
    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            if (!doc.isNull() && doc.isObject()) {
                Invoice inv = Invoice::fromJson(doc.object());
                loadInvoiceToUi(inv);
                QMessageBox::information(this, "Éxito", "Factura cargada correctamente.");
            } else {
                QMessageBox::warning(this, "Aviso", "El archivo no contiene un formato de factura válido.");
            }
        }
    }
}
