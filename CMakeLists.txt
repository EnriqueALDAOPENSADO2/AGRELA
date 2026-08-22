cmake_minimum_required(VERSION 3.16)
project(AGRELA_Facturacion VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Garantizar que usa libstdc++ del sistema en Linux
if(UNIX AND NOT APPLE)
    set(CMAKE_BUILD_RPATH "/usr/lib64")
    set(CMAKE_INSTALL_RPATH "/usr/lib64")
    link_directories(/usr/lib64)
endif()

find_package(Qt5 REQUIRED COMPONENTS Widgets PrintSupport Gui Core)

include_directories(src/utils)

set(MINIZ_SOURCES
    src/utils/miniz.c
    src/utils/miniz_tdef.c
    src/utils/miniz_tinfl.c
    src/utils/miniz_zip.c
    src/utils/ExcelReader.h
    src/utils/ExcelReader.cpp
)

set(SOURCES
    src/main.cpp
    ${MINIZ_SOURCES}
    src/models/CatalogItem.h
    src/models/CatalogItem.cpp
    src/models/InvoiceItem.h
    src/models/InvoiceItem.cpp
    src/models/Invoice.h
    src/models/Invoice.cpp
    src/services/CatalogService.h
    src/services/CatalogService.cpp
    src/services/ClientService.h
    src/services/ClientService.cpp
    src/services/InvoiceGeneratorService.h
    src/services/InvoiceGeneratorService.cpp
    src/ui/LineItemDialog.h
    src/ui/LineItemDialog.cpp
    src/ui/ClientSelectorDialog.h
    src/ui/ClientSelectorDialog.cpp
    src/ui/CatalogWidget.h
    src/ui/CatalogWidget.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
)

set(RESOURCES
    resources.qrc
)

if(WIN32)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/windows/app.rc")
        list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/windows/app.rc")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/windows/app_icon.rc")
        list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/windows/app_icon.rc")
    endif()
endif()

add_executable(agrela_facturacion
    ${SOURCES}
    ${RESOURCES}
)

if(WIN32)
    set_target_properties(agrela_facturacion PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

target_link_libraries(agrela_facturacion PRIVATE
    Qt5::Widgets
    Qt5::PrintSupport
    Qt5::Gui
    Qt5::Core
)

add_executable(test_render_ui
    test_render_ui.cpp
    ${MINIZ_SOURCES}
    src/models/CatalogItem.cpp
    src/models/InvoiceItem.cpp
    src/models/Invoice.cpp
    src/services/CatalogService.cpp
    src/services/ClientService.cpp
    src/services/InvoiceGeneratorService.cpp
    src/ui/LineItemDialog.cpp
    src/ui/ClientSelectorDialog.cpp
    src/ui/CatalogWidget.cpp
    src/ui/MainWindow.cpp
)

target_link_libraries(test_render_ui PRIVATE
    Qt5::Widgets
    Qt5::PrintSupport
    Qt5::Gui
    Qt5::Core
)

add_executable(test_runner
    test_runner.cpp
    ${MINIZ_SOURCES}
    src/models/CatalogItem.cpp
    src/models/InvoiceItem.cpp
    src/models/Invoice.cpp
    src/services/CatalogService.cpp
    src/services/ClientService.cpp
    src/services/InvoiceGeneratorService.cpp
)

target_link_libraries(test_runner PRIVATE
    Qt5::Widgets
    Qt5::PrintSupport
    Qt5::Gui
    Qt5::Core
)
