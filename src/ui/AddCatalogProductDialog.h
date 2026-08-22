#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include "../models/CatalogItem.h"

class AddCatalogProductDialog : public QDialog {
    Q_OBJECT

public:
    explicit AddCatalogProductDialog(QWidget* parent = nullptr);
    CatalogItem getProduct() const;

private slots:
    void onPvpChanged(double value);
    void onCalcT1Clicked();
    void onSaveClicked();

private:
    void setupUi();

    QComboBox* m_cmbSheet = nullptr;
    QLineEdit* m_txtCategory = nullptr;
    QLineEdit* m_txtCode = nullptr;
    QLineEdit* m_txtDesc = nullptr;
    QDoubleSpinBox* m_spnPvp = nullptr;
    QDoubleSpinBox* m_spnT1 = nullptr;
    QComboBox* m_cmbUnidad = nullptr;
    QPushButton* m_btnSave = nullptr;
    QPushButton* m_btnCancel = nullptr;
};
