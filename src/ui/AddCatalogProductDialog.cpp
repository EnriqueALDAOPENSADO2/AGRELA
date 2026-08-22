#include "AddCatalogProductDialog.h"
#include <cmath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QDateTime>
#include "../services/CatalogService.h"

AddCatalogProductDialog::AddCatalogProductDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Añadir Nuevo Producto al Catálogo");
    setMinimumWidth(520);
    setStyleSheet(
        "QDialog { background-color: #F8FAFC; color: #1E293B; }"
        "QLabel { color: #1E293B; font-size: 13px; font-weight: 600; }"
        "QLineEdit, QDoubleSpinBox, QComboBox { background-color: #FFFFFF; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 6px; padding: 6px 10px; font-size: 13px; }"
        "QLineEdit:focus, QDoubleSpinBox:focus, QComboBox:focus { border: 1.5px solid #2B78C5; }"
    );
    setupUi();
}

void AddCatalogProductDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(14);

    auto* grp = new QGroupBox("Datos del Producto", this);
    grp->setStyleSheet(
        "QGroupBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; margin-top: 10px; font-weight: bold; color: #1F4E78; font-size: 13px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 14px; padding: 0 6px; background-color: #FFFFFF; }"
    );

    auto* formLayout = new QFormLayout(grp);
    formLayout->setContentsMargins(16, 20, 16, 16);
    formLayout->setSpacing(12);

    m_cmbSheet = new QComboBox(this);
    m_cmbSheet->setEditable(true);
    m_cmbSheet->addItems({"Flexol", "Mosquiflex", "Accesorios", "Guías y Perfiles", "Lamas y Cajones", "Motores y Automatismos", "Persianas Enrollables", "Venecianas y Graduables", "Personalizada"});

    m_txtCategory = new QLineEdit(this);
    m_txtCategory->setPlaceholderText("Ej: Mosquiteras Enrollables, Cortinas Plisadas, etc.");

    m_txtCode = new QLineEdit(this);
    m_txtCode->setPlaceholderText("Ej: FLX-001, MQ-001 (dejar en blanco para auto-generar)");

    m_txtDesc = new QLineEdit(this);
    m_txtDesc->setPlaceholderText("Nombre / Descripción completa del producto");

    m_spnPvp = new QDoubleSpinBox(this);
    m_spnPvp->setRange(0.0, 999999.0);
    m_spnPvp->setDecimals(2);
    m_spnPvp->setSuffix(" €");
    m_spnPvp->setValue(0.0);

    m_spnT1 = new QDoubleSpinBox(this);
    m_spnT1->setRange(0.0, 999999.0);
    m_spnT1->setDecimals(2);
    m_spnT1->setSuffix(" €");
    m_spnT1->setValue(0.0);

    auto* btnCalcT1 = new QPushButton("⚡ -25% T1", this);
    btnCalcT1->setCursor(Qt::PointingHandCursor);
    btnCalcT1->setToolTip("Calcular Tarifa 1 automáticamente con 25% de descuento sobre el PVP");
    btnCalcT1->setStyleSheet(
        "QPushButton { background-color: #EBF5FB; color: #1F4E78; border: 1px solid #2B78C5; border-radius: 5px; padding: 5px 8px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background-color: #D9E1F2; }"
    );
    connect(btnCalcT1, &QPushButton::clicked, this, &AddCatalogProductDialog::onCalcT1Clicked);

    auto* priceT1Layout = new QHBoxLayout();
    priceT1Layout->addWidget(m_spnT1, 1);
    priceT1Layout->addWidget(btnCalcT1);

    m_cmbUnidad = new QComboBox(this);
    m_cmbUnidad->addItems({"ud.", "m²", "ml."});

    formLayout->addRow("Sección / Catálogo:", m_cmbSheet);
    formLayout->addRow("Familia / Subcategoría:", m_txtCategory);
    formLayout->addRow("Código:", m_txtCode);
    formLayout->addRow("Descripción:", m_txtDesc);
    formLayout->addRow("Precio PVP (€):", m_spnPvp);
    formLayout->addRow("Tarifa 1 - T-1 (€):", priceT1Layout);
    formLayout->addRow("Unidad de Venta:", m_cmbUnidad);

    mainLayout->addWidget(grp);

    // Botonera inferior
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_btnCancel = new QPushButton("Cancelar", this);
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    m_btnCancel->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #64748B; border: 1px solid #CBD5E1; border-radius: 6px; padding: 8px 16px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    m_btnSave = new QPushButton("💾 Guardar en Catálogo", this);
    m_btnSave->setCursor(Qt::PointingHandCursor);
    m_btnSave->setStyleSheet(
        "QPushButton { background-color: #1F4E78; color: #FFFFFF; border: none; border-radius: 6px; padding: 8px 18px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #163857; }"
    );
    connect(m_btnSave, &QPushButton::clicked, this, &AddCatalogProductDialog::onSaveClicked);

    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnSave);
    mainLayout->addLayout(btnLayout);

    connect(m_spnPvp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AddCatalogProductDialog::onPvpChanged);
}

void AddCatalogProductDialog::onPvpChanged(double value) {
    if (m_spnT1->value() == 0.0 || m_spnT1->value() == value) {
        m_spnT1->setValue(value);
    }
}

void AddCatalogProductDialog::onCalcT1Clicked() {
    double pvp = m_spnPvp->value();
    m_spnT1->setValue(std::round(pvp * 0.75 * 100.0) / 100.0);
}

void AddCatalogProductDialog::onSaveClicked() {
    QString desc = m_txtDesc->text().trimmed();
    if (desc.isEmpty()) {
        QMessageBox::warning(this, "Campo Obligatorio", "Por favor introduce la descripción o nombre del producto.");
        m_txtDesc->setFocus();
        return;
    }

    if (m_spnPvp->value() <= 0.0) {
        QMessageBox::warning(this, "Precio Requerido", "Por favor indica un precio PVP válido superior a 0 €.");
        m_spnPvp->setFocus();
        return;
    }

    accept();
}

CatalogItem AddCatalogProductDialog::getProduct() const {
    CatalogItem it;
    QString sheet = m_cmbSheet->currentText().trimmed();
    if (sheet.isEmpty()) sheet = "Personalizada";
    it.sheet = sheet;

    QString cat = m_txtCategory->text().trimmed();
    if (cat.isEmpty()) cat = sheet;
    it.category = cat;

    QString code = m_txtCode->text().trimmed();
    if (code.isEmpty()) {
        QString prefix = sheet.left(3).toUpper();
        code = QString("%1-%2").arg(prefix, QString::number(QDateTime::currentDateTime().toSecsSinceEpoch() % 100000));
    }
    it.code = code;
    it.desc = m_txtDesc->text().trimmed();
    it.pvp = m_spnPvp->value();
    it.p1 = it.pvp;
    
    double t1 = m_spnT1->value();
    if (t1 <= 0.0) t1 = it.pvp;
    it.t1 = t1;
    it.p_t1 = t1;

    it.u1 = m_cmbUnidad->currentText();
    return it;
}
