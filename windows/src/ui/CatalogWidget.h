#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QVector>
#include "../models/CatalogItem.h"

class CatalogWidget : public QWidget {
    Q_OBJECT

public:
    explicit CatalogWidget(QWidget* parent = nullptr);

signals:
    void itemSelected(const CatalogItem& item);

private slots:
    void onSearchChanged();
    void onCategoryChanged(int index);
    void onRowDoubleClicked(int row, int col);
    void onAddClicked();
    void onRefreshPreciosClicked();

private:
    void setupUi();
    void populateTable(const QVector<CatalogItem>& items);
    void refreshCategoryCombo();

    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_categoryCombo = nullptr;
    QLabel* m_lblCount = nullptr;
    QTableWidget* m_table = nullptr;
    QPushButton* m_btnAdd = nullptr;
    QPushButton* m_btnRefresh = nullptr;

    QVector<CatalogItem> m_currentItems;
};
