#include "ExcelReader.h"
#include <QXmlStreamReader>
#include <QFile>
#include <QDebug>
#include <cstring>

extern "C" {
#include "miniz.h"
}

static int columnNameToIndex(const QString& colName) {
    int col = 0;
    for (int i = 0; i < colName.length(); ++i) {
        QChar c = colName[i].toUpper();
        if (c >= 'A' && c <= 'Z') {
            col = col * 26 + (c.toLatin1() - 'A' + 1);
        }
    }
    return col - 1; // 0-based
}

static int parseCellColumn(const QString& cellRef) {
    QString colName;
    for (int i = 0; i < cellRef.length(); ++i) {
        if (cellRef[i].isLetter()) {
            colName.append(cellRef[i]);
        } else {
            break;
        }
    }
    return columnNameToIndex(colName);
}

QVector<QStringList> ExcelReader::readXlsx(const QString& xlsxPath, const QString& sheetFileName) {
    QVector<QStringList> rows;
    if (!QFile::exists(xlsxPath)) {
        return rows;
    }

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, xlsxPath.toLocal8Bit().constData(), 0)) {
        qWarning() << "ExcelReader: No se pudo abrir el archivo zip .xlsx:" << xlsxPath;
        return rows;
    }

    // 1. Extraer xl/sharedStrings.xml si existe
    QVector<QString> sharedStrings;
    int ssIdx = mz_zip_reader_locate_file(&zip, "xl/sharedStrings.xml", nullptr, 0);
    if (ssIdx >= 0) {
        size_t ssSize = 0;
        void* ssData = mz_zip_reader_extract_to_heap(&zip, ssIdx, &ssSize, 0);
        if (ssData) {
            QByteArray ssXml(reinterpret_cast<const char*>(ssData), static_cast<int>(ssSize));
            mz_free(ssData);

            QXmlStreamReader xml(ssXml);
            QString currentStr;
            bool insideSi = false;
            bool insideT = false;

            while (!xml.atEnd() && !xml.hasError()) {
                xml.readNext();
                if (xml.isStartElement()) {
                    if (xml.name() == QLatin1String("si")) {
                        insideSi = true;
                        currentStr.clear();
                    } else if (insideSi && xml.name() == QLatin1String("t")) {
                        insideT = true;
                    }
                } else if (xml.isCharacters() && insideSi && insideT) {
                    currentStr.append(xml.text());
                } else if (xml.isEndElement()) {
                    if (xml.name() == QLatin1String("t")) {
                        insideT = false;
                    } else if (xml.name() == QLatin1String("si")) {
                        sharedStrings.append(currentStr);
                        insideSi = false;
                    }
                }
            }
        }
    }

    // 2. Extraer la hoja especificada (ej. xl/worksheets/sheet1.xml)
    int sheetIdx = mz_zip_reader_locate_file(&zip, sheetFileName.toLocal8Bit().constData(), 0, 0);
    if (sheetIdx < 0) {
        // Si no se encuentra con la ruta exacta, buscar cualquier sheet1.xml
        sheetIdx = mz_zip_reader_locate_file(&zip, "xl/worksheets/sheet1.xml", 0, 0);
    }

    if (sheetIdx >= 0) {
        size_t sheetSize = 0;
        void* sheetData = mz_zip_reader_extract_to_heap(&zip, sheetIdx, &sheetSize, 0);
        if (sheetData) {
            QByteArray sheetXml(reinterpret_cast<const char*>(sheetData), static_cast<int>(sheetSize));
            mz_free(sheetData);

            QXmlStreamReader xml(sheetXml);
            QStringList currentRow;
            QString cellType;
            QString cellRef;
            QString cellValue;
            bool insideV = false;
            bool insideIsT = false;

            while (!xml.atEnd() && !xml.hasError()) {
                xml.readNext();
                if (xml.isStartElement()) {
                    if (xml.name() == QLatin1String("row")) {
                        currentRow.clear();
                    } else if (xml.name() == QLatin1String("c")) {
                        cellType = xml.attributes().value("t").toString();
                        cellRef = xml.attributes().value("r").toString();
                        cellValue.clear();
                    } else if (xml.name() == QLatin1String("v")) {
                        insideV = true;
                    } else if (xml.name() == QLatin1String("t")) {
                        insideIsT = true;
                    }
                } else if (xml.isCharacters()) {
                    if (insideV || insideIsT) {
                        cellValue.append(xml.text());
                    }
                } else if (xml.isEndElement()) {
                    if (xml.name() == QLatin1String("v")) {
                        insideV = false;
                    } else if (xml.name() == QLatin1String("t")) {
                        insideIsT = false;
                    } else if (xml.name() == QLatin1String("c")) {
                        int colIdx = parseCellColumn(cellRef);
                        while (currentRow.size() <= colIdx) {
                            currentRow.append("");
                        }
                        if (cellType == "s") {
                            int sIndex = cellValue.toInt();
                            if (sIndex >= 0 && sIndex < sharedStrings.size()) {
                                currentRow[colIdx] = sharedStrings[sIndex];
                            } else {
                                currentRow[colIdx] = cellValue;
                            }
                        } else {
                            currentRow[colIdx] = cellValue;
                        }
                    } else if (xml.name() == QLatin1String("row")) {
                        rows.append(currentRow);
                    }
                }
            }
        }
    }

    mz_zip_reader_end(&zip);
    return rows;
}
