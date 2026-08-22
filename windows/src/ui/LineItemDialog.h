#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include "../models/InvoiceItem.h"
#include "../models/CatalogItem.h"

class LineItemDialog : public QDialog {
    Q_OBJECT

public:
    explicit LineItemDialog(const InvoiceItem& item, QWidget* parent = nullptr);
    InvoiceItem getItem() const;

private slots:
    void updateCalculations();
    void onUnidadChanged(int index);
    void onTariffToggled();
    void onColoresEspecialesToggled(bool checked);

private:
    void setupUi();

    InvoiceItem m_item;
    CatalogItem m_catItem;

    QLineEdit* m_txtDesc = nullptr;
    QLineEdit* m_txtCode = nullptr;
    
    QRadioButton* m_radPvp = nullptr;
    QRadioButton* m_radT1 = nullptr;
    QButtonGroup* m_tariffGroup = nullptr;
    QCheckBox* m_chkColoresEspeciales = nullptr;

    QComboBox* m_cmbUnidad = nullptr;
    QDoubleSpinBox* m_spnUnidades = nullptr;
    QDoubleSpinBox* m_spnPrecio = nullptr;
    QDoubleSpinBox* m_spnAnchoFinal = nullptr;
    QDoubleSpinBox* m_spnAnchoRollo = nullptr;
    QDoubleSpinBox* m_spnAlto = nullptr;

    QLabel* m_lblFinalHelp = nullptr;
    QLabel* m_lblRolloHelp = nullptr;
    QLabel* m_lblAltoHelp = nullptr;

    QLabel* m_lblM2Calc = nullptr;
    QLabel* m_lblTotalCalc = nullptr;
    QLabel* m_lblImgPreview = nullptr;
};
