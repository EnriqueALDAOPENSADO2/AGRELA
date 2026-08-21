#!/usr/bin/env bash
# Script de lanzamiento de AGRELA Facturación
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

# Asegurar carga de bibliotecas del sistema para Qt / GCC
export LD_PRELOAD=/usr/lib64/libstdc++.so.6

exec "$DIR/build/agrela_facturacion" "$@"
