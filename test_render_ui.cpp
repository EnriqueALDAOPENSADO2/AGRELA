#include <QApplication>
#include <QPixmap>
#include <QDebug>
#include <QStyleFactory>
#include "src/ui/MainWindow.h"
#include "src/services/CatalogService.h"

int main(int argc, char* argv[]) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    QApplication app(argc, argv);
    CatalogService::instance().loadCatalog("catalog.json");

    MainWindow window;
    window.resize(1380, 860);
    window.show();

    // Renderizar widget a imagen
    QPixmap pixmap(window.size());
    window.render(&pixmap);
    pixmap.save("test_ui_render.png");
    qDebug() << "Captura de la ventana guardada en test_ui_render.png";

    return 0;
}
