#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include "../models/InvoiceItem.h"

class LineItemDialog : public QDialog {
    Q_OBJECT

public:
    explicit LineItemDialog(const InvoiceItem& item, QWidget* parent = nullptr);
    InvoiceItem getItem() const;

private slots:
    void updateCalculations();
    void onUnidadChanged(int index);

private:
    void setupUi();

    InvoiceItem m_item;

    QLineEdit* m_txtDesc = nullptr;
    QLineEdit* m_txtCode = nullptr;
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
