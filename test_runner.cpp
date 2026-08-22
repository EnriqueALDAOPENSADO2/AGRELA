#include <iostream>
#include <cassert>
#include <cmath>
#include <QApplication>
#include <QDate>
#include <QDebug>
#include "src/models/Invoice.h"
#include "src/models/InvoiceItem.h"
#include "src/services/CatalogService.h"
#include "src/services/ClientService.h"
#include "src/services/InvoiceGeneratorService.h"
#include "src/services/TransactionService.h"

bool approxEqual(double a, double b, double eps = 0.001) {
    return std::abs(a - b) < eps;
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    std::cout << "========================================" << std::endl;
    std::cout << "  AGRELA TEST RUNNER (CÁLCULOS Y REGLAS)" << std::endl;
    std::cout << "========================================" << std::endl;

    // Test 1: Persianas con cálculo por superficie M² y ancho de rollo vs final
    {
        InvoiceItem persiana;
        persiana.desc = "Aluminio de 43 mm. color blanco";
        persiana.unidades = 2.0;
        persiana.precioUnitario = 32.00; // 32 €/m²
        persiana.anchoPersianaFinal = 1200.0; // 1200 mm para el cliente
        persiana.anchoRolloUsado = 1300.0;     // 1300 mm rollo cobrado
        persiana.alto = 1500.0;               // 1500 mm
        persiana.unidad = "m²";

        double unitM2 = persiana.calcularMetrosCuadradosUnitario();
        double totalM2 = persiana.calcularMetrosCuadrados();
        double total = persiana.calcularTotal();

        std::cout << "[Test 1: Persiana por M²]" << std::endl;
        std::cout << "  Uds: " << persiana.unidades << std::endl;
        std::cout << "  Ancho Final (factura): " << persiana.anchoPersianaFinal << " mm" << std::endl;
        std::cout << "  Ancho Rollo (cálculo): " << persiana.anchoRolloUsado << " mm" << std::endl;
        std::cout << "  Alto: " << persiana.alto << " mm" << std::endl;
        std::cout << "  M² 1 Ud: " << unitM2 << " m² (esperado: 1.950)" << std::endl;
        std::cout << "  M² Total Facturado: " << totalM2 << " m² (esperado: 3.900)" << std::endl;
        std::cout << "  Total Línea: " << total << " € (esperado: 124.80 €)" << std::endl;

        assert(approxEqual(unitM2, 1.950));
        assert(approxEqual(totalM2, 3.900));
        assert(approxEqual(total, 124.80));
        std::cout << "  => PASSED!\n" << std::endl;
    }

    // Test 2: Productos por Unidad (Motores / Accesorios)
    {
        InvoiceItem motor;
        motor.desc = "MOTOR BLUE ROLL 9/16 L35 (Eje de 40) 21 Kg.";
        motor.unidades = 3.0;
        motor.precioUnitario = 100.00; // 100 €/ud
        motor.anchoPersianaFinal = 0.0;
        motor.anchoRolloUsado = 0.0;
        motor.alto = 0.0;
        motor.unidad = "ud.";

        double m2 = motor.calcularMetrosCuadrados();
        double total = motor.calcularTotal();

        std::cout << "[Test 2: Producto por Unidad]" << std::endl;
        std::cout << "  Uds: " << motor.unidades << std::endl;
        std::cout << "  Precio/ud: " << motor.precioUnitario << " €" << std::endl;
        std::cout << "  M²: " << m2 << " (esperado: 0)" << std::endl;
        std::cout << "  Total: " << total << " € (esperado: 300.00 €)" << std::endl;

        assert(approxEqual(m2, 0.0));
        assert(approxEqual(total, 300.00));
        std::cout << "  => PASSED!\n" << std::endl;
    }

    // Test 3: Productos por Metro Lineal (Guías / Perfiles)
    {
        InvoiceItem guia;
        guia.desc = "Guia de recubrimiento Lacado BLANCO con aleta";
        guia.unidades = 2.0;
        guia.precioUnitario = 4.32; // 4.32 €/ml
        guia.anchoPersianaFinal = 2500.0; // 2.50 metros
        guia.anchoRolloUsado = 2500.0;
        guia.alto = 0.0;
        guia.unidad = "ml.";

        double mlTotal = guia.calcularMetrosCuadrados();
        double total = guia.calcularTotal();

        std::cout << "[Test 3: Producto por Metro Lineal]" << std::endl;
        std::cout << "  Uds: " << guia.unidades << " de " << guia.anchoPersianaFinal << " mm" << std::endl;
        std::cout << "  Metros Lineales Totales: " << mlTotal << " ml (esperado: 5.000)" << std::endl;
        std::cout << "  Total: " << total << " € (esperado: 21.60 €)" << std::endl;

        assert(approxEqual(mlTotal, 5.000));
        assert(approxEqual(total, 21.60));
        std::cout << "  => PASSED!\n" << std::endl;
    }

    // Test 4: Generación completa de factura con los 3 tipos de productos
    {
        Invoice inv;
        inv.cliente.nombre = "VENTANAS CORISTANCO S. L.";
        inv.cliente.direccion = "RUA DO COBRE PARCELA C - 5 B";
        inv.cliente.poblacion = "POLIGONO DE BERTOA CARBALLO C. P. 15 100";
        inv.cliente.provincia = "A CORUÑA";
        inv.cliente.cifNif = "B - 70 193 248";
        inv.numeroFactura = "2026-CALC";
        inv.fecha = QDate::currentDate();
        inv.formaPago = "Giro bancario 30 días";
        inv.tipoIva = 0.21;

        // Persiana por m² (2 uds de 1200x1500 con rollo de 1300)
        InvoiceItem item1;
        item1.desc = "Aluminio de 43 mm. color blanco";
        item1.unidades = 2.0;
        item1.precioUnitario = 32.00;
        item1.anchoPersianaFinal = 1200.0;
        item1.anchoRolloUsado = 1300.0;
        item1.alto = 1500.0;
        item1.unidad = "m²";
        inv.items.append(item1);

        // Motor por unidad (1 ud a 100 €)
        InvoiceItem item2;
        item2.desc = "MOTOR BLUE ROLL 9/16 L35";
        item2.unidades = 1.0;
        item2.precioUnitario = 100.00;
        item2.unidad = "ud.";
        inv.items.append(item2);

        // Guía por metro lineal (2 uds de 2500 mm a 4.32 €/ml)
        InvoiceItem item3;
        item3.desc = "Guia de recubrimiento Lacado BLANCO";
        item3.unidades = 2.0;
        item3.precioUnitario = 4.32;
        item3.anchoPersianaFinal = 2500.0;
        item3.anchoRolloUsado = 2500.0;
        item3.unidad = "ml.";
        inv.items.append(item3);

        double brutoEsperado = 124.80 + 100.00 + 21.60; // 246.40 €
        double ivaEsperado = 51.74;                     // 51.74 € (redondeado a 2 decimales)
        double totalEsperado = 298.14;                  // 298.14 € (redondeado a 2 decimales)

        std::cout << "[Test 4: Factura Completa Multi-Tipo]" << std::endl;
        std::cout << "  Bruto calculado: " << inv.calcularTotalBruto() << " € (esperado: " << brutoEsperado << " €)" << std::endl;
        std::cout << "  IVA calculado: " << inv.calcularCuotaIva() << " € (esperado: " << ivaEsperado << " €)" << std::endl;
        std::cout << "  Total factura: " << inv.calcularTotalFactura() << " € (esperado: " << totalEsperado << " €)" << std::endl;

        assert(approxEqual(inv.calcularTotalBruto(), brutoEsperado));
        assert(approxEqual(inv.calcularCuotaIva(), ivaEsperado));
        assert(approxEqual(inv.calcularTotalFactura(), totalEsperado));

        auto files = InvoiceGeneratorService::instance().generateBoth(inv, "facturas");
        std::cout << "  Excel generado nativo: " << files.first.toStdString() << std::endl;
        std::cout << "  PDF generado nativo: " << files.second.toStdString() << std::endl;
        assert(!files.first.isEmpty());
        assert(!files.second.isEmpty());
        std::cout << "  => PASSED!\n" << std::endl;
    }

    // Test 5: Verificación de Tarifa PVP vs Tarifa 1 (T-1)
    {
        std::cout << "[Test 5: Verificación de Tarifa PVP vs Tarifa 1 (T-1)]" << std::endl;

        CatalogItem catItem;
        catItem.code = "501000130";
        catItem.desc = "Recogedor embutir de plástico grande";
        catItem.pvp = 4.50;
        catItem.t1 = 3.70;
        catItem.u1 = "ud.";

        InvoiceItem itemPvp = InvoiceItem::fromCatalogItem(catItem, "PVP");
        itemPvp.unidades = 10.0;
        assert(approxEqual(itemPvp.precioUnitario, 4.50));
        assert(approxEqual(itemPvp.calcularTotal(), 45.00));

        InvoiceItem itemT1 = InvoiceItem::fromCatalogItem(catItem, "T1");
        itemT1.unidades = 10.0;
        assert(approxEqual(itemT1.precioUnitario, 3.70));
        assert(approxEqual(itemT1.calcularTotal(), 37.00));

        std::cout << "  Precio PVP (10 uds): " << itemPvp.calcularTotal() << " € (esperado: 45.00 €)" << std::endl;
        std::cout << "  Precio T-1 (10 uds): " << itemT1.calcularTotal() << " € (esperado: 37.00 €)" << std::endl;
        std::cout << "  => PASSED!\n" << std::endl;
    }

    // Test 6: Producto Manual Fuera de Catálogo con Dimensiones Opcionales
    {
        std::cout << "[Test 6: Producto Manual Fuera de Catálogo con Dimensiones Opcionales]" << std::endl;

        InvoiceItem manualItem;
        manualItem.code = "MAN-001";
        manualItem.desc = "Mosquitera Enrollable Mosquiflex 42 Blanco (Fuera de Catálogo)";
        manualItem.unidades = 2.0;
        manualItem.precioUnitario = 48.00; // 48 € por unidad
        manualItem.unidad = "ud.";
        manualItem.anchoPersianaFinal = 1150.0; // Dimensión opcional informada (1150 mm)
        manualItem.alto = 1400.0;               // Dimensión opcional informada (1400 mm)

        double total = manualItem.calcularTotal();
        std::cout << "  Uds: " << manualItem.unidades << " a " << manualItem.precioUnitario << " €/ud" << std::endl;
        std::cout << "  Medidas opcionales: " << manualItem.anchoPersianaFinal << " x " << manualItem.alto << " mm" << std::endl;
        std::cout << "  Total calculado: " << total << " € (esperado: 96.00 €)" << std::endl;

        assert(approxEqual(total, 96.00));
        assert(approxEqual(manualItem.anchoPersianaFinal, 1150.0));
        assert(approxEqual(manualItem.alto, 1400.0));
        std::cout << "  => PASSED!\n" << std::endl;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "  TODOS LOS TESTS DE CÁLCULO PASARON!   " << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
