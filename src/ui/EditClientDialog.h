#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include "../models/Invoice.h"

class EditClientDialog : public QDialog {
    Q_OBJECT

public:
    explicit EditClientDialog(const Customer& client = Customer(), QWidget* parent = nullptr);
    Customer getClient() const;

private slots:
    void onSaveClicked();

private:
    void setupUi();

    QLineEdit* m_txtAlias = nullptr;
    QLineEdit* m_txtNombre = nullptr;
    QLineEdit* m_txtCif = nullptr;
    QLineEdit* m_txtDireccion = nullptr;
    QLineEdit* m_txtPoblacion = nullptr;
    QLineEdit* m_txtProvincia = nullptr;
    QLineEdit* m_txtTelefono = nullptr;
    QLineEdit* m_txtEmail = nullptr;

    Customer m_client;
    bool m_isNew = true;
};
