#pragma once

#include <QVector>
#include <QStringList>
#include "../models/CatalogItem.h"

class CatalogService {
public:
    static CatalogService& instance();

    bool loadCatalog(const QString& jsonPath = "catalog.json");
    bool loadFromExcel(const QString& xlsxPath = "precios_catalogo.xlsx");
    bool syncWithPreciosFolder(const QString& folderPath = "PRECIOS", const QString& jsonPath = "catalog.json", bool force = false);
    
    bool addCustomItem(const CatalogItem& item, const QString& jsonPath = "catalog.json", const QString& xlsxPath = "precios_catalogo.xlsx");
    bool saveCatalog(const QString& jsonPath = "catalog.json", const QString& xlsxPath = "precios_catalogo.xlsx");

    const QVector<CatalogItem>& getAllItems() const;
    QStringList getSheets() const;
    QStringList getCategoriesForSheet(const QString& sheet) const;
    
    QVector<CatalogItem> search(const QString& text, const QString& sheetFilter = "Todas") const;
    CatalogItem findByCode(const QString& code) const;

private:
    CatalogService() = default;
    void runExtractionScript(const QString& folderPath, const QString& jsonPath);

    QVector<CatalogItem> m_items;
};
