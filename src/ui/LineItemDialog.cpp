#include "LineItemDialog.h"
#include "../services/CatalogService.h"
#include <cmath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPixmap>
#include <QFile>

LineItemDialog::LineItemDialog(const InvoiceItem& item, QWidget* parent)
    : QDialog(parent), m_item(item) {
    m_catItem = CatalogService::instance().findByCode(m_item.code);
    
    if (m_item.code.isEmpty() || m_item.code == "MANUAL") {
        setWindowTitle("Añadir Producto Manual a la Factura (Fuera de Catálogo)");
    } else {
        setWindowTitle("Configurar Artículo y Dimensiones (mm)");
    }

    setMinimumWidth(640);
    setStyleSheet("background-color: #F8FAFC; color: #1E293B;");
    setupUi();
    onUnidadChanged(m_cmbUnidad->currentIndex());
    updateCalculations();
}

void LineItemDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Cabecera con imagen y descripción
    auto* headerCard = new QFrame(this);
    headerCard->setStyleSheet("background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; padding: 8px;");
    auto* headerLayout = new QHBoxLayout(headerCard);

    m_lblImgPreview = new QLabel(this);
    m_lblImgPreview->setFixedSize(72, 72);
    m_lblImgPreview->setAlignment(Qt::AlignCenter);
    m_lblImgPreview->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px;");

    if (!m_item.imgPath.isEmpty() && QFile::exists(m_item.imgPath)) {
        QPixmap pix(m_item.imgPath);
        m_lblImgPreview->setPixmap(pix.scaled(66, 66, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_lblImgPreview->setText(m_item.code.isEmpty() ? "Manual" : "Sin Croquis");
        m_lblImgPreview->setStyleSheet("color: #94A3B8; font-size: 11px; font-weight: bold; background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px;");
    }
    headerLayout->addWidget(m_lblImgPreview);

    auto* headerTextLayout = new QVBoxLayout();
    m_txtCode = new QLineEdit(m_item.code, this);
    m_txtCode->setPlaceholderText("Código / Referencia");
    if (!m_item.code.isEmpty() && !m_catItem.code.isEmpty()) {
        m_txtCode->setReadOnly(true);
        m_txtCode->setStyleSheet("font-weight: bold; color: #1F4E78; background: transparent; border: none; font-size: 13px;");
    } else {
        m_txtCode->setReadOnly(false);
        m_txtCode->setStyleSheet("background-color: #FFFFFF; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 4px; padding: 3px 6px; font-size: 12px; font-weight: 600;");
    }

    QString initialDesc = m_item.desc;
    if (initialDesc == "Artículo personalizado") initialDesc = "";
    m_txtDesc = new QLineEdit(initialDesc, this);
    m_txtDesc->setPlaceholderText("Introduce el nombre o descripción del producto (ej: Mosquitera Mosquiflex, Cortina Flexol, etc.)...");
    m_txtDesc->setStyleSheet("background-color: #FFFFFF; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 5px; padding: 6px 8px; font-size: 13px; font-weight: 600;");

    headerTextLayout->addWidget(m_txtCode);
    headerTextLayout->addWidget(m_txtDesc);
    headerLayout->addLayout(headerTextLayout);

    mainLayout->addWidget(headerCard);

    // Formulario de valores
    auto* formGroup = new QGroupBox("Parámetros y Tipo de Facturación", this);
    formGroup->setStyleSheet(
        "QGroupBox { background-color: #FFFFFF; border: 1.5px solid #CBD5E1; border-radius: 8px; margin-top: 10px; font-weight: bold; color: #1F4E78; font-size: 13px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 14px; padding: 0 6px; background-color: #FFFFFF; }"
        "QLabel { color: #1E293B; font-size: 12px; font-weight: 600; }"
        "QDoubleSpinBox, QComboBox { background-color: #FFFFFF; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 5px; padding: 6px 8px; font-size: 13px; font-weight: 500; }"
        "QDoubleSpinBox:focus, QComboBox:focus { border: 1.5px solid #2B78C5; }"
        "QDoubleSpinBox:disabled { background-color: #F1F5F9; color: #94A3B8; border: 1px solid #E2E8F0; }"
        "QRadioButton { font-size: 12px; font-weight: 600; color: #1E293B; padding: 3px 6px; }"
    );

    auto* formLayout = new QFormLayout(formGroup);
    formLayout->setContentsMargins(14, 18, 14, 14);
    formLayout->setSpacing(10);

    // Selector de Tarifa: PVP vs T1
    auto* tariffLayout = new QHBoxLayout();
    double pvpDisplay = (m_catItem.pvp > 0.0) ? m_catItem.pvp : ((m_catItem.p1 > 0.0) ? m_catItem.p1 : m_item.precioUnitario);
    double t1Display = (m_catItem.t1 > 0.0) ? m_catItem.t1 : ((m_catItem.p_t1 > 0.0) ? m_catItem.p_t1 : pvpDisplay);

    if (m_catItem.code.isEmpty()) {
        m_radPvp = new QRadioButton("PVP (Tarifa General)", this);
        m_radT1 = new QRadioButton("Tarifa 1 - T-1", this);
    } else {
        m_radPvp = new QRadioButton(QString("PVP (%1 €)").arg(pvpDisplay, 0, 'f', 2), this);
        m_radT1 = new QRadioButton(QString("Tarifa 1 - T1 (%1 €)").arg(t1Display, 0, 'f', 2), this);
    }

    m_tariffGroup = new QButtonGroup(this);
    m_tariffGroup->addButton(m_radPvp);
    m_tariffGroup->addButton(m_radT1);

    if (m_item.tarifa.compare("T1", Qt::CaseInsensitive) == 0 || m_item.tarifa.compare("T-1", Qt::CaseInsensitive) == 0) {
        m_radT1->setChecked(true);
    } else {
        m_radPvp->setChecked(true);
    }

    auto* btnQuickT1 = new QPushButton("⚡ Aplicar -25% T1", this);
    btnQuickT1->setCursor(Qt::PointingHandCursor);
    btnQuickT1->setToolTip("Calcular rápidamente el precio con 25% de descuento");
    btnQuickT1->setStyleSheet(
        "QPushButton { background-color: #EBF5FB; color: #1F4E78; border: 1px solid #2B78C5; border-radius: 5px; padding: 4px 8px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background-color: #D9E1F2; }"
    );
    connect(btnQuickT1, &QPushButton::clicked, [this]() {
        double current = m_spnPrecio->value();
        m_spnPrecio->setValue(std::round(current * 0.75 * 100.0) / 100.0);
        m_radT1->setChecked(true);
        updateCalculations();
    });

    tariffLayout->addWidget(m_radPvp);
    tariffLayout->addWidget(m_radT1);
    tariffLayout->addWidget(btnQuickT1);
    tariffLayout->addStretch();
    formLayout->addRow("Tarifa de Precio:", tariffLayout);

    // Selector de Tipo de Tarificación / Unidad
    m_cmbUnidad = new QComboBox(this);
    m_cmbUnidad->addItem("Por Superficie (m²)", "m²");
    m_cmbUnidad->addItem("Por Unidad Fija (ud.)", "ud.");
    m_cmbUnidad->addItem("Por Metro Lineal (ml.)", "ml.");

    QString uClean = m_item.unidad.toLower().trimmed();
    if (uClean.contains("ud") || uClean.contains("unid")) {
        m_cmbUnidad->setCurrentIndex(1);
    } else if (uClean.contains("ml") || uClean.contains("metro")) {
        m_cmbUnidad->setCurrentIndex(2);
    } else {
        m_cmbUnidad->setCurrentIndex(0);
    }

    // Uds y Precio
    m_spnUnidades = new QDoubleSpinBox(this);
    m_spnUnidades->setRange(0.01, 9999.0);
    m_spnUnidades->setDecimals(0);
    m_spnUnidades->setValue(m_item.unidades > 0 ? m_item.unidades : 1.0);

    m_spnPrecio = new QDoubleSpinBox(this);
    m_spnPrecio->setRange(0.0, 999999.0);
    m_spnPrecio->setDecimals(2);
    m_spnPrecio->setSuffix(" €");
    m_spnPrecio->setValue(m_item.precioUnitario);

    formLayout->addRow("Tipo de Tarificación / Unidad:", m_cmbUnidad);
    formLayout->addRow("Número de Unidades:", m_spnUnidades);
    formLayout->addRow("Precio Unitario:", m_spnPrecio);

    // Ancho Persiana Final vs Ancho Rollo Usado (en mm)
    m_spnAnchoFinal = new QDoubleSpinBox(this);
    m_spnAnchoFinal->setRange(0.0, 10000.0);
    m_spnAnchoFinal->setDecimals(0);
    m_spnAnchoFinal->setSingleStep(10);
    m_spnAnchoFinal->setSuffix(" mm");
    m_spnAnchoFinal->setValue(m_item.anchoPersianaFinal);

    m_spnAnchoRollo = new QDoubleSpinBox(this);
    m_spnAnchoRollo->setRange(0.0, 10000.0);
    m_spnAnchoRollo->setDecimals(0);
    m_spnAnchoRollo->setSingleStep(10);
    m_spnAnchoRollo->setSuffix(" mm");
    m_spnAnchoRollo->setValue(m_item.anchoRolloUsado > 0 ? m_item.anchoRolloUsado : m_item.anchoPersianaFinal);

    m_spnAlto = new QDoubleSpinBox(this);
    m_spnAlto->setRange(0.0, 10000.0);
    m_spnAlto->setDecimals(0);
    m_spnAlto->setSingleStep(10);
    m_spnAlto->setSuffix(" mm");
    m_spnAlto->setValue(m_item.alto);

    m_lblFinalHelp = new QLabel("<b>Ancho Persiana Final (mm):</b><br><span style='color:#64748B; font-weight:normal;'>Medida instalada de la persiana acabada (aparece en la factura).</span>", this);
    formLayout->addRow(m_lblFinalHelp, m_spnAnchoFinal);

    m_lblRolloHelp = new QLabel("<b>Ancho Rollo Usado (mm):</b><br><span style='color:#C2410C; font-weight:normal;'>Ancho del corte/rollo cobrado (se usa para calcular los M²).</span>", this);
    formLayout->addRow(m_lblRolloHelp, m_spnAnchoRollo);

    m_lblAltoHelp = new QLabel("<b>Alto Persiana (mm):</b>", this);
    formLayout->addRow(m_lblAltoHelp, m_spnAlto);

    mainLayout->addWidget(formGroup);

    // Resumen de cálculos en tiempo real
    auto* summaryCard = new QFrame(this);
    summaryCard->setStyleSheet("background-color: #EBF5FB; border: 1.5px solid #2B78C5; border-radius: 8px; padding: 10px;");
    auto* summaryLayout = new QVBoxLayout(summaryCard);

    m_lblM2Calc = new QLabel("Superficie Total: 0.000 m²", this);
    m_lblM2Calc->setStyleSheet("font-size: 13px; font-weight: bold; color: #1F4E78;");

    m_lblTotalCalc = new QLabel("Total Línea: 0.00 €", this);
    m_lblTotalCalc->setStyleSheet("font-size: 16px; font-weight: bold; color: #1F4E78;");

    summaryLayout->addWidget(m_lblM2Calc);
    summaryLayout->addWidget(m_lblTotalCalc);
    mainLayout->addWidget(summaryCard);

    // Botones OK / Cancel
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btnBox->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #1E293B; border: 1px solid #CBD5E1; border-radius: 6px; padding: 7px 18px; font-weight: 600; font-size: 13px; min-width: 85px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
        "QPushButton[default=\"true\"] { background-color: #2B78C5; color: #FFFFFF; border: none; }"
        "QPushButton[default=\"true\"]:hover { background-color: #1F5F9F; }"
    );
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(btnBox);

    // Conectar señales para recálculo instantáneo
    connect(m_txtDesc, &QLineEdit::textChanged, this, &LineItemDialog::updateCalculations);
    connect(m_radPvp, &QRadioButton::toggled, this, &LineItemDialog::onTariffToggled);
    connect(m_radT1, &QRadioButton::toggled, this, &LineItemDialog::onTariffToggled);
    connect(m_cmbUnidad, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LineItemDialog::onUnidadChanged);
    connect(m_spnUnidades, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineItemDialog::updateCalculations);
    connect(m_spnPrecio, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineItemDialog::updateCalculations);
    connect(m_spnAnchoFinal, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double val){
        if (m_spnAnchoRollo->isEnabled() && m_spnAnchoRollo->value() == 0.0) {
            m_spnAnchoRollo->setValue(val);
        }
        updateCalculations();
    });
    connect(m_spnAnchoRollo, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineItemDialog::updateCalculations);
    connect(m_spnAlto, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LineItemDialog::updateCalculations);
}

void LineItemDialog::onTariffToggled() {
    if (m_radT1 && m_radT1->isChecked()) {
        m_item.tarifa = "T1";
        if (m_catItem.t1 > 0.0) {
            m_spnPrecio->setValue(m_catItem.t1);
        } else if (m_catItem.p_t1 > 0.0) {
            m_spnPrecio->setValue(m_catItem.p_t1);
        }
    } else {
        m_item.tarifa = "PVP";
        if (m_catItem.pvp > 0.0) {
            m_spnPrecio->setValue(m_catItem.pvp);
        } else if (m_catItem.p1 > 0.0) {
            m_spnPrecio->setValue(m_catItem.p1);
        }
    }
    updateCalculations();
}

void LineItemDialog::onUnidadChanged(int index) {
    QString uType = m_cmbUnidad->itemData(index).toString();
    m_item.unidad = uType;

    if (uType == "ud.") {
        m_spnAnchoFinal->setEnabled(false);
        m_spnAnchoRollo->setEnabled(false);
        m_spnAlto->setEnabled(false);

        m_spnAnchoFinal->setValue(0.0);
        m_spnAnchoRollo->setValue(0.0);
        m_spnAlto->setValue(0.0);

        m_lblFinalHelp->setText("<b>Ancho (mm):</b> <span style='color:#94A3B8; font-weight:normal;'>(No aplicable para venta por unidad)</span>");
        m_lblRolloHelp->setText("<b>Ancho Rollo (mm):</b> <span style='color:#94A3B8; font-weight:normal;'>(No aplicable para venta por unidad)</span>");
        m_lblAltoHelp->setText("<b>Alto (mm):</b> <span style='color:#94A3B8; font-weight:normal;'>(No aplicable para venta por unidad)</span>");
    } else if (uType == "ml.") {
        m_spnAnchoFinal->setEnabled(true);
        m_spnAnchoRollo->setEnabled(true);
        m_spnAlto->setEnabled(false);

        m_spnAlto->setValue(0.0);

        m_lblFinalHelp->setText("<b>Longitud Pieza (mm):</b> <span style='color:#64748B; font-weight:normal;'>Largo por unidad en milímetros.</span>");
        m_lblRolloHelp->setText("<b>Longitud Rollo (mm):</b> <span style='color:#C2410C; font-weight:normal;'>Longitud facturada si difiere.</span>");
        m_lblAltoHelp->setText("<b>Alto (mm):</b> <span style='color:#94A3B8; font-weight:normal;'>(No aplicable para perfiles lineales)</span>");
    } else {
        m_spnAnchoFinal->setEnabled(true);
        m_spnAnchoRollo->setEnabled(true);
        m_spnAlto->setEnabled(true);

        m_lblFinalHelp->setText("<b>Ancho Persiana Final (mm):</b><br><span style='color:#64748B; font-weight:normal;'>Medida instalada de la persiana acabada (aparece en la factura).</span>");
        m_lblRolloHelp->setText("<b>Ancho Rollo Usado (mm):</b><br><span style='color:#C2410C; font-weight:normal;'>Ancho del corte/rollo cobrado (se usa para calcular los M²).</span>");
        m_lblAltoHelp->setText("<b>Alto Persiana (mm):</b>");
    }
    updateCalculations();
}

void LineItemDialog::updateCalculations() {
    m_item.desc = m_txtDesc ? m_txtDesc->text().trimmed() : m_item.desc;
    m_item.unidades = m_spnUnidades ? m_spnUnidades->value() : 1.0;
    m_item.precioUnitario = m_spnPrecio ? m_spnPrecio->value() : 0.0;
    m_item.anchoPersianaFinal = m_spnAnchoFinal ? m_spnAnchoFinal->value() : 0.0;
    m_item.anchoRolloUsado = m_spnAnchoRollo ? m_spnAnchoRollo->value() : 0.0;
    m_item.alto = m_spnAlto ? m_spnAlto->value() : 0.0;
    m_item.tarifa = (m_radT1 && m_radT1->isChecked()) ? "T1" : "PVP";

    double totalM2 = m_item.calcularMetrosCuadrados();
    double totalEuros = m_item.calcularTotal();

    if (m_item.unidad == "m²") {
        double unitM2 = m_item.calcularMetrosCuadradosUnitario();
        m_lblM2Calc->setText(QString("Superficie Total: %1 m² (%2 uds x %3 m² / ud)")
            .arg(totalM2, 0, 'f', 3)
            .arg(m_item.unidades, 0, 'f', 0)
            .arg(unitM2, 0, 'f', 3));
    } else if (m_item.unidad == "ml.") {
        double unitMl = m_item.calcularMetrosCuadradosUnitario();
        m_lblM2Calc->setText(QString("Metros Lineales Totales: %1 ml (%2 uds x %3 ml / ud)")
            .arg(totalM2, 0, 'f', 3)
            .arg(m_item.unidades, 0, 'f', 0)
            .arg(unitMl, 0, 'f', 3));
    } else {
        m_lblM2Calc->setText(QString("Venta por unidad fija (%1 uds a %2 €/ud)")
            .arg(m_item.unidades, 0, 'f', 0)
            .arg(m_item.precioUnitario, 0, 'f', 2));
    }

    m_lblTotalCalc->setText(QString("Total Línea: %1 €").arg(totalEuros, 0, 'f', 2));
}

InvoiceItem LineItemDialog::getItem() const {
    InvoiceItem res = m_item;
    if (m_txtCode) res.code = m_txtCode->text().trimmed();
    if (m_txtDesc) res.desc = m_txtDesc->text().trimmed();
    if (m_spnUnidades) res.unidades = m_spnUnidades->value();
    if (m_spnPrecio) res.precioUnitario = m_spnPrecio->value();
    if (m_spnAnchoFinal) res.anchoPersianaFinal = m_spnAnchoFinal->value();
    if (m_spnAnchoRollo) res.anchoRolloUsado = m_spnAnchoRollo->value();
    if (m_spnAlto) res.alto = m_spnAlto->value();
    if (m_cmbUnidad) res.unidad = m_cmbUnidad->currentData().toString();
    if (m_radT1) res.tarifa = m_radT1->isChecked() ? "T1" : "PVP";
    return res;
}
