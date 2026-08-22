#include "EditClientDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

EditClientDialog::EditClientDialog(const Customer& client, QWidget* parent)
    : QDialog(parent), m_client(client), m_isNew(client.nombre.isEmpty() && client.alias.isEmpty()) {
    
    setWindowTitle(m_isNew ? "➕ Añadir Nuevo Cliente" : "✏️ Modificar Datos del Cliente");
    resize(520, 420);
    setStyleSheet("background-color: #FFFFFF; color: #1E293B;");

    setupUi();
}

void EditClientDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    // Título
    auto* lblTitle = new QLabel(m_isNew ? "<h3>Nuevo Cliente</h3>" : "<h3>Editar Datos del Cliente</h3>", this);
    lblTitle->setStyleSheet("color: #1F4E78; margin-bottom: 6px;");
    layout->addWidget(lblTitle);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);
    formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QString inputStyle = 
        "QLineEdit { background-color: #F8FAFC; color: #1E293B; border: 1.5px solid #CBD5E1; border-radius: 5px; padding: 6px 10px; font-size: 13px; }"
        "QLineEdit:focus { border: 1.5px solid #2B78C5; background-color: #FFFFFF; }";

    m_txtAlias = new QLineEdit(m_client.alias, this);
    m_txtAlias->setPlaceholderText("Ej: VENCORIS, ALMAGAL (código corto o alias)");
    m_txtAlias->setStyleSheet(inputStyle);

    m_txtNombre = new QLineEdit(m_client.nombre, this);
    m_txtNombre->setPlaceholderText("Nombre completo o Razón Social (ej: VENTANAS CORISTANCO S.L.)");
    m_txtNombre->setStyleSheet(inputStyle);

    m_txtCif = new QLineEdit(m_client.cifNif, this);
    m_txtCif->setPlaceholderText("CIF / NIF (ej: B-70193248)");
    m_txtCif->setStyleSheet(inputStyle);

    m_txtDireccion = new QLineEdit(m_client.direccion, this);
    m_txtDireccion->setPlaceholderText("Calle, número, polígono...");
    m_txtDireccion->setStyleSheet(inputStyle);

    m_txtPoblacion = new QLineEdit(m_client.poblacion, this);
    m_txtPoblacion->setPlaceholderText("Población y Código Postal (ej: CARBALLO C.P. 15100)");
    m_txtPoblacion->setStyleSheet(inputStyle);

    m_txtProvincia = new QLineEdit(m_client.provincia.isEmpty() ? "A CORUÑA" : m_client.provincia, this);
    m_txtProvincia->setPlaceholderText("Provincia (ej: A CORUÑA)");
    m_txtProvincia->setStyleSheet(inputStyle);

    m_txtTelefono = new QLineEdit(m_client.telefono, this);
    m_txtTelefono->setPlaceholderText("Teléfono de contacto (opcional)");
    m_txtTelefono->setStyleSheet(inputStyle);

    m_txtEmail = new QLineEdit(m_client.email, this);
    m_txtEmail->setPlaceholderText("Correo electrónico (opcional)");
    m_txtEmail->setStyleSheet(inputStyle);

    QString labelStyle = "font-weight: bold; color: #334155; font-size: 12px;";
    auto makeLabel = [&](const QString& txt) {
        auto* l = new QLabel(txt, this);
        l->setStyleSheet(labelStyle);
        return l;
    };

    formLayout->addRow(makeLabel("Alias / Código:"), m_txtAlias);
    formLayout->addRow(makeLabel("Razón Social *:"), m_txtNombre);
    formLayout->addRow(makeLabel("CIF / NIF:"), m_txtCif);
    formLayout->addRow(makeLabel("Dirección:"), m_txtDireccion);
    formLayout->addRow(makeLabel("Población / CP:"), m_txtPoblacion);
    formLayout->addRow(makeLabel("Provincia:"), m_txtProvincia);
    formLayout->addRow(makeLabel("Teléfono:"), m_txtTelefono);
    formLayout->addRow(makeLabel("Email:"), m_txtEmail);

    layout->addLayout(formLayout);

    // Botones Guardar y Cancelar
    auto* btnLayout = new QHBoxLayout();
    auto* btnCancel = new QPushButton("Cancelar", this);
    btnCancel->setStyleSheet(
        "QPushButton { background-color: #F1F5F9; color: #334155; border: 1px solid #CBD5E1; border-radius: 6px; padding: 8px 16px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background-color: #E2E8F0; }"
    );

    auto* btnSave = new QPushButton("💾 Guardar Cliente", this);
    btnSave->setCursor(Qt::PointingHandCursor);
    btnSave->setStyleSheet(
        "QPushButton { background-color: #16A34A; color: #FFFFFF; font-weight: bold; font-size: 13px; padding: 8px 20px; border-radius: 6px; border: none; }"
        "QPushButton:hover { background-color: #15803D; }"
        "QPushButton:pressed { background-color: #166534; }"
    );

    btnLayout->addWidget(btnCancel);
    btnLayout->addStretch();
    btnLayout->addWidget(btnSave);
    layout->addLayout(btnLayout);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnSave, &QPushButton::clicked, this, &EditClientDialog::onSaveClicked);
}

void EditClientDialog::onSaveClicked() {
    QString nombre = m_txtNombre->text().trimmed();
    QString alias = m_txtAlias->text().trimmed();

    if (nombre.isEmpty() && alias.isEmpty()) {
        QMessageBox::warning(this, "Aviso", "Debes indicar al menos el Nombre / Razón Social o el Alias del cliente.");
        return;
    }

    if (alias.isEmpty()) {
        alias = nombre;
    }
    if (nombre.isEmpty()) {
        nombre = alias;
    }

    m_client.alias = alias;
    m_client.nombre = nombre;
    m_client.cifNif = m_txtCif->text().trimmed();
    m_client.direccion = m_txtDireccion->text().trimmed();
    m_client.poblacion = m_txtPoblacion->text().trimmed();
    m_client.provincia = m_txtProvincia->text().trimmed();
    m_client.telefono = m_txtTelefono->text().trimmed();
    m_client.email = m_txtEmail->text().trimmed();

    accept();
}

Customer EditClientDialog::getClient() const {
    return m_client;
}
