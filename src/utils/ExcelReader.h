#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

class ExcelReader {
public:
    // Lee una hoja de un archivo .xlsx y devuelve una lista de filas con sus columnas
    static QVector<QStringList> readXlsx(const QString& xlsxPath, const QString& sheetFileName = "xl/worksheets/sheet1.xml");
};
