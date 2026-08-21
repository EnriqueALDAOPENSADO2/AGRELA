#include <QApplication>
#include <QIcon>
#include <QFont>
#include <QFile>
#include <QDir>
#include <QStyleFactory>
#include "ui/MainWindow.h"
#include "services/CatalogService.h"

int main(int argc, char* argv[]) {
    // Configurar estilo base Fusion para consistencia en cualquier tema de escritorio
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QApplication app(argc, argv);
    app.setApplicationName("Persianas A Grela");
    app.setApplicationDisplayName("Persianas A Grela - Gestión y Facturación");
    app.setOrganizationName("Persianas A Grela");

    // Icono de la aplicación
    QIcon appIcon(":/app_icon.png");
    if (appIcon.isNull()) appIcon = QIcon("app_icon.png");
    if (appIcon.isNull()) appIcon = QIcon("logo.jpg");
    app.setWindowIcon(appIcon);

    // Tipografía moderna
    QFont font("Segoe UI", 10);
    font.setStyleHint(QFont::SansSerif);
    app.setFont(font);

    // Estilos globales de la aplicación blindados contra inconsistencias de temas oscuros/claros
    app.setStyleSheet(
        "QMainWindow { background-color: #F1F5F9; }"
        "QWidget { font-family: 'Segoe UI', 'Helvetica Neue', Arial, sans-serif; color: #1E293B; }"
        "QToolTip { background-color: #1E293B; color: #FFFFFF; border: 1px solid #334155; padding: 4px; border-radius: 4px; font-size: 12px; }"
        "QLineEdit, QDateEdit, QDoubleSpinBox, QSpinBox {"
        "   background-color: #FFFFFF;"
        "   color: #1E293B;"
        "   border: 1.5px solid #CBD5E1;"
        "   border-radius: 6px;"
        "   padding: 5px 8px;"
        "   font-size: 13px;"
        "}"
        "QLineEdit:focus, QDateEdit:focus, QDoubleSpinBox:focus {"
        "   border: 1.5px solid #2B78C5;"
        "}"
        "QComboBox {"
        "   background-color: #FFFFFF;"
        "   color: #1E293B;"
        "   border: 1.5px solid #CBD5E1;"
        "   border-radius: 6px;"
        "   padding: 5px 8px;"
        "   font-size: 13px;"
        "}"
        "QComboBox QAbstractItemView {"
        "   background-color: #FFFFFF;"
        "   color: #1E293B;"
        "   selection-background-color: #D9E1F2;"
        "   selection-color: #1F4E78;"
        "   border: 1px solid #CBD5E1;"
        "   outline: none;"
        "}"
        "QPushButton {"
        "   background-color: #F1F5F9;"
        "   color: #334155;"
        "   border: 1px solid #CBD5E1;"
        "   border-radius: 6px;"
        "   padding: 6px 14px;"
        "   font-weight: 600;"
        "   font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #E2E8F0;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #CBD5E1;"
        "}"
        "QGroupBox {"
        "   background-color: #FFFFFF;"
        "   border: 1.5px solid #CBD5E1;"
        "   border-radius: 8px;"
        "   margin-top: 10px;"
        "   padding-top: 12px;"
        "   font-weight: bold;"
        "   color: #1F4E78;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top left;"
        "   left: 14px;"
        "   padding: 0 6px;"
        "   background-color: #FFFFFF;"
        "}"
        "QScrollBar:vertical {"
        "   border: none;"
        "   background: #F1F5F9;"
        "   width: 8px;"
        "   border-radius: 4px;"
        "}"
        "QScrollBar:horizontal {"
        "   border: none;"
        "   background: #F1F5F9;"
        "   height: 8px;"
        "   border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal {"
        "   background: #CBD5E1;"
        "   border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover {"
        "   background: #94A3B8;"
        "}"
    );

    // Cargar y Sincronizar Catálogo con la carpeta PRECIOS
    CatalogService::instance().syncWithPreciosFolder("PRECIOS", "catalog.json");

    MainWindow window;
    window.show();

    return app.exec();
}
