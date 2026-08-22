#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include "../models/Invoice.h"

class ClientSelectorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ClientSelectorDialog(QWidget* parent = nullptr);
    Customer getSelectedClient() const;

private slots:
    void onSearchChanged();
    void onRowDoubleClicked(int row, int col);
    void onSelectClicked();
    void onRefreshFromFolderClicked();
    void onAddClientClicked();
    void onEditClientClicked();
    void onDeleteClientClicked();

private:
    void setupUi();
    void populateTable(const QVector<Customer>& clients);

    QLineEdit* m_searchEdit = nullptr;
    QLabel* m_lblCount = nullptr;
    QTableWidget* m_table = nullptr;
    QPushButton* m_btnSelect = nullptr;
    QPushButton* m_btnRefresh = nullptr;
    QPushButton* m_btnAdd = nullptr;
    QPushButton* m_btnEdit = nullptr;
    QPushButton* m_btnDelete = nullptr;

    QVector<Customer> m_currentList;
    Customer m_selectedClient;
};
