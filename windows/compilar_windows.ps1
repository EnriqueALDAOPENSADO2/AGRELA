Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "  COMPILADOR PERSIANAS A GRELA PARA WINDOWS (C++ Qt5)" -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path -Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location build
Write-Host "[1/3] Configurando CMake para Windows..." -ForegroundColor Yellow
cmake .. -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake no pudo configurar el proyecto." -ForegroundColor Red
    Write-Host "Asegúrate de tener instalado Qt5 / Qt6 y CMake en tu sistema." -ForegroundColor Red
    Pause
    exit 1
}

Write-Host "`n[2/3] Compilando ejecutable agrela_facturacion.exe..." -ForegroundColor Yellow
cmake --build . --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Falló la compilación de C++." -ForegroundColor Red
    Pause
    exit 1
}

Set-Location ..

Write-Host "`n[3/3] Empaquetando aplicación portable en 'AGRELA_Windows_Portable'..." -ForegroundColor Yellow
if (-not (Test-Path -Path "AGRELA_Windows_Portable")) {
    New-Item -ItemType Directory -Path "AGRELA_Windows_Portable" | Out-Null
}

if (Test-Path "build\Release\agrela_facturacion.exe") {
    Copy-Item -Force "build\Release\agrela_facturacion.exe" "AGRELA_Windows_Portable\"
} elseif (Test-Path "build\agrela_facturacion.exe") {
    Copy-Item -Force "build\agrela_facturacion.exe" "AGRELA_Windows_Portable\"
}

Copy-Item -Force "sample.xlsx" "AGRELA_Windows_Portable\"
Copy-Item -Force "catalog.json" "AGRELA_Windows_Portable\"
Copy-Item -Force "clientes.json" "AGRELA_Windows_Portable\"
if (Test-Path "logo.jpg") { Copy-Item -Force "logo.jpg" "AGRELA_Windows_Portable\" }
if (Test-Path "logo.jpeg") { Copy-Item -Force "logo.jpeg" "AGRELA_Windows_Portable\" }
if (Test-Path "app_icon.ico") { Copy-Item -Force "app_icon.ico" "AGRELA_Windows_Portable\" }
if (Test-Path "app_icon.png") { Copy-Item -Force "app_icon.png" "AGRELA_Windows_Portable\" }
if (Test-Path "extracted_images") { Copy-Item -Recurse -Force "extracted_images" "AGRELA_Windows_Portable\" }
if (Test-Path "PRECIOS") { Copy-Item -Recurse -Force "PRECIOS" "AGRELA_Windows_Portable\" }
if (Test-Path "CLIENTES") { Copy-Item -Recurse -Force "CLIENTES" "AGRELA_Windows_Portable\" }

Write-Host "Desplegando librerías DLL de Qt (windeployqt)..." -ForegroundColor Yellow
windeployqt AGRELA_Windows_Portable\agrela_facturacion.exe

Write-Host "`n========================================================" -ForegroundColor Green
Write-Host "  ¡COMPILACIÓN Y EMPAQUETADO COMPLETADOS CON ÉXITO!" -ForegroundColor Green
Write-Host "  La carpeta 'AGRELA_Windows_Portable' contiene el .exe" -ForegroundColor Green
Write-Host "  y todos los datos listos para ejecutarse." -ForegroundColor Green
Write-Host "========================================================" -ForegroundColor Green
Write-Host ""
Pause
