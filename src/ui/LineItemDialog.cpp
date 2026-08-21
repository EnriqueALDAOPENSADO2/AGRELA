#include "LineItemDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPixmap>
#include <QFile>

LineItemDialog::LineItemDialog(const InvoiceItem& item, QWidget* parent)
    : QDialog(parent), m_item(item) {
    setWindowTitle("Configurar Artículo y Dimensiones (mm)");
    setMinimumWidth(620);
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
        m_lblImgPreview->setText("Sin Croquis");
        m_lblImgPreview->setStyleSheet("color: #94A3B8; font-size: 11px; font-weight: bold; background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 6px;");
    }
    headerLayout->addWidget(m_lblImgPreview);

    auto* headerTextLayout = new QVBoxLayout();
    m_txtCode = new QLineEdit(m_item.code, this);
    m_txtCode->setPlaceholderText("Código");
    m_txtCode->setReadOnly(true);
    m_txtCode->setStyleSheet("font-weight: bold; color: #1F4E78; background: transparent; border: none; font-size: 13px;");

    m_txtDesc = new QLineEdit(m_item.desc, this);
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
    );

    auto* formLayout = new QFormLayout(formGroup);
    formLayout->setContentsMargins(14, 18, 14, 14);
    formLayout->setSpacing(10);

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
    formLayout->addRow("Precio Unitario PVP:", m_spnPrecio);

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

    // Resumen de Cálculo
    auto* calcCard = new QFrame(this);
    calcCard->setStyleSheet("background-color: #F0FDF4; border: 1.5px solid #86EFAC; border-radius: 8px; padding: 12px;");
    auto* calcLayout = new QVBoxLayout(calcCard);
    calcLayout->setSpacing(6);

    m_lblM2Calc = new QLabel("Superficie Calculada: 0.00 m²", this);
    m_lblM2Calc->setStyleSheet("font-size: 13px; color: #166534; font-weight: bold;");

    m_lblTotalCalc = new QLabel("Total Línea: 0.00 €", this);
    m_lblTotalCalc->setStyleSheet("font-size: 18px; color: #14532D; font-weight: bold;");

    calcLayout->addWidget(m_lblM2Calc);
    calcLayout->addWidget(m_lblTotalCalc);

    mainLayout->addWidget(calcCard);

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

void LineItemDialog::onUnidadChanged(int index) {
    QString uType = m_cmbUnidad->itemData(index).toString();
    m_item.unidad = uType;

    if (uType == "ud.") {
        // Productos por Unidad: DESHABILITAR Ancho y Largo
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
        // Productos por Metro Lineal: HABILITAR Ancho/Longitud, DESHABILITAR Alto
        m_spnAnchoFinal->setEnabled(true);
        m_spnAnchoRollo->setEnabled(true);
        m_spnAlto->setEnabled(false);

        m_spnAlto->setValue(0.0);

        m_lblFinalHelp->setText("<b>Longitud Perfil (mm):</b><br><span style='color:#64748B; font-weight:normal;'>Longitud por unidad en milímetros.</span>");
        m_lblRolloHelp->setText("<b>Longitud Cobrada (mm):</b><br><span style='color:#C2410C; font-weight:normal;'>Longitud del corte facturado en milímetros.</span>");
        m_lblAltoHelp->setText("<b>Alto (mm):</b> <span style='color:#94A3B8; font-weight:normal;'>(No aplicable a metro lineal)</span>");
    } else {
        // Productos por Superficie (m²): HABILITAR UDS, ANCHO Y LARGO
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
    m_item.desc = m_txtDesc->text().trimmed();
    m_item.unidad = m_cmbUnidad->currentData().toString();
    m_item.unidades = m_spnUnidades->value();
    m_item.precioUnitario = m_spnPrecio->value();

    if (m_item.unidad == "ud.") {
        m_item.anchoPersianaFinal = 0.0;
        m_item.anchoRolloUsado = 0.0;
        m_item.alto = 0.0;
    } else {
        m_item.anchoPersianaFinal = m_spnAnchoFinal->value();
        m_item.anchoRolloUsado = m_spnAnchoRollo->value();
        m_item.alto = (m_item.unidad == "ml.") ? 0.0 : m_spnAlto->value();
    }

    double unitM2 = m_item.calcularMetrosCuadradosUnitario();
    double totalM2 = m_item.calcularMetrosCuadrados();
    double total = m_item.calcularTotal();

    if (m_item.unidad == "m²") {
        double wCobrado = (m_item.anchoRolloUsado > 0) ? m_item.anchoRolloUsado : m_item.anchoPersianaFinal;
        m_lblM2Calc->setText(QString("📐 Medidas por Persiana: <b>%1 mm</b> (rollo) × <b>%2 mm</b> (alto) = <b>%3 m² / ud.</b><br>Superficie Total Facturada: <b>%4 uds × %3 m² = %5 m²</b>").arg(
            QString::number(wCobrado, 'f', 0),
            QString::number(m_item.alto, 'f', 0),
            QString::number(unitM2, 'f', 3),
            QString::number(m_item.unidades, 'f', 0),
            QString::number(totalM2, 'f', 3)
        ));
        m_lblTotalCalc->setText(QString("💰 Total Línea: <b>%1 €</b> (%2 m² × %3 €/m²)").arg(
            QString::number(total, 'f', 2),
            QString::number(totalM2, 'f', 3),
            QString::number(m_item.precioUnitario, 'f', 2)
        ));
    } else if (m_item.unidad == "ml.") {
        double lCobrado = (m_item.anchoRolloUsado > 0) ? m_item.anchoRolloUsado : m_item.anchoPersianaFinal;
        m_lblM2Calc->setText(QString("📏 Longitud por Perfil: <b>%1 mm</b> (Total: <b>%2 uds × %3 mm = %4 ml</b>)").arg(
            QString::number(lCobrado, 'f', 0),
            QString::number(m_item.unidades, 'f', 0),
            QString::number(lCobrado, 'f', 0),
            QString::number(totalM2, 'f', 2)
        ));
        m_lblTotalCalc->setText(QString("💰 Total Línea: <b>%1 €</b> (%2 ml × %3 €/ml)").arg(
            QString::number(total, 'f', 2),
            QString::number(totalM2, 'f', 2),
            QString::number(m_item.precioUnitario, 'f', 2)
        ));
    } else {
        m_lblM2Calc->setText(QString("📦 Facturación Directa por Unidad: <b>%1 uds</b>").arg(
            QString::number(m_item.unidades, 'f', 0)
        ));
        m_lblTotalCalc->setText(QString("💰 Total Línea: <b>%1 €</b> (%2 uds × %3 €/ud)").arg(
            QString::number(total, 'f', 2),
            QString::number(m_item.unidades, 'f', 0),
            QString::number(m_item.precioUnitario, 'f', 2)
        ));
    }
}

InvoiceItem LineItemDialog::getItem() const {
    InvoiceItem res = m_item;
    res.desc = m_txtDesc->text().trimmed();
    res.unidad = m_cmbUnidad->currentData().toString();
    res.unidades = m_spnUnidades->value();
    res.precioUnitario = m_spnPrecio->value();

    if (res.unidad == "ud.") {
        res.anchoPersianaFinal = 0.0;
        res.anchoRolloUsado = 0.0;
        res.alto = 0.0;
    } else {
        res.anchoPersianaFinal = m_spnAnchoFinal->value();
        res.anchoRolloUsado = m_spnAnchoRollo->value();
        res.alto = (res.unidad == "ml.") ? 0.0 : m_spnAlto->value();
    }
    return res;
}
