@echo off
echo ========================================================
echo   COMPILADOR PERSIANAS A GRELA PARA WINDOWS (C++ Qt5)
echo ========================================================
echo.

if not exist build mkdir build

cd build
echo [1/3] Configurando CMake para Windows...
cmake .. -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [ERROR] CMake no pudo configurar el proyecto.
    echo Asegurate de tener instalado Qt5 / Qt6 y CMake en tu sistema.
    pause
    exit /b 1
)

echo.
echo [2/3] Compilando ejecutable agrela_facturacion.exe...
cmake --build . --config Release -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo [ERROR] Fallo la compilacion de C++.
    pause
    exit /b 1
)

cd ..

echo.
echo [3/3] Empaquetando aplicacion portable en 'AGRELA_Windows_Portable'...
if not exist AGRELA_Windows_Portable mkdir AGRELA_Windows_Portable

if exist build\Release\agrela_facturacion.exe (
    copy /y build\Release\agrela_facturacion.exe AGRELA_Windows_Portable\
) else if exist build\agrela_facturacion.exe (
    copy /y build\agrela_facturacion.exe AGRELA_Windows_Portable\
)

copy /y sample.xlsx AGRELA_Windows_Portable\
if exist precios_catalogo.xlsx copy /y precios_catalogo.xlsx AGRELA_Windows_Portable\
if exist clientes.xlsx copy /y clientes.xlsx AGRELA_Windows_Portable\
copy /y catalog.json AGRELA_Windows_Portable\
copy /y clientes.json AGRELA_Windows_Portable\
if exist logo.jpg copy /y logo.jpg AGRELA_Windows_Portable\
if exist logo.jpeg copy /y logo.jpeg AGRELA_Windows_Portable\
if exist app_icon.ico copy /y app_icon.ico AGRELA_Windows_Portable\
if exist app_icon.png copy /y app_icon.png AGRELA_Windows_Portable\
if exist extracted_images xcopy /E /I /Y extracted_images AGRELA_Windows_Portable\extracted_images >nul 2>&1
if exist PRECIOS xcopy /E /I /Y PRECIOS AGRELA_Windows_Portable\PRECIOS >nul 2>&1
if exist "CARPETA CLIENTES" xcopy /E /I /Y "CARPETA CLIENTES" "AGRELA_Windows_Portable\CARPETA CLIENTES" >nul 2>&1

echo Desplegando librerias DLL de Qt (windeployqt)...
windeployqt AGRELA_Windows_Portable\agrela_facturacion.exe >nul 2>&1

echo.
echo ========================================================
echo   COMPILACION Y EMPAQUETADO COMPLETADOS CON EXITO!
echo   La carpeta 'AGRELA_Windows_Portable' contiene el .exe
echo   y todos los datos listos para ejecutarse.
echo ========================================================
echo.
pause
