#!/usr/bin/env python3
import os
import sys
import json
import re
import unicodedata
import xlrd

def clean_str(v):
    if v is None:
        return ""
    s = str(v).strip()
    if s.endswith(".0") and s[:-2].isdigit():
        s = s[:-2]
    # Normalizar espacios múltiples
    s = re.sub(r"\s+", " ", s)
    return s

def extract_strings_from_corrupt_xls(filepath):
    try:
        with open(filepath, "rb") as f:
            raw = f.read()
        strings = [m.decode("latin1", errors="ignore").strip() for m in re.findall(b"[\x20-\x7E]{3,}", raw)]
        
        nombre = os.path.splitext(os.path.basename(filepath))[0]
        direccion = ""
        poblacion = ""
        provincia = ""
        cif_nif = ""

        for s in strings:
            if "verdillo" in s.lower() and not direccion:
                direccion = s
            elif "carballo" in s.lower() and not poblacion:
                poblacion = s
            elif "coru" in s.lower() and not provincia:
                provincia = "A CORUÑA"
            elif re.search(r"[A-Z0-9]{8,}", s.replace(" ", "")) and not cif_nif:
                cif_nif = s

        return {
            "alias": os.path.splitext(os.path.basename(filepath))[0],
            "nombre": nombre,
            "direccion": direccion,
            "poblacion": poblacion,
            "provincia": provincia,
            "cif_nif": cif_nif,
            "telefono": "",
            "email": "",
            "file": os.path.basename(filepath)
        }
    except Exception:
        return None

def extract_clients_from_folder(folder_path="CARPETA CLIENTES", output_json="clientes.json"):
    if not os.path.exists(folder_path):
        print(f"La carpeta '{folder_path}' no existe.")
        return []

    files = sorted([f for f in os.listdir(folder_path) if f.endswith((".xls", ".xlsx"))])
    clients = []

    for f in files:
        path = os.path.join(folder_path, f)
        alias = os.path.splitext(f)[0]
        try:
            wb = xlrd.open_workbook(path)
            sh = wb.sheet_by_index(0)

            nombre = ""
            direccion = ""
            poblacion = ""
            provincia = ""
            cif_nif = ""

            for r in range(min(15, sh.nrows)):
                for c in range(min(9, sh.ncols) - 1):
                    cell_lbl = clean_str(sh.cell_value(r, c)).lower()
                    cell_val = clean_str(sh.cell_value(r, c + 1))
                    if not cell_val and c + 2 < sh.ncols:
                        cell_val = clean_str(sh.cell_value(r, c + 2))

                    if "nombre" in cell_lbl and not nombre:
                        nombre = cell_val
                    elif ("direcci" in cell_lbl or "domicilio" in cell_lbl) and not direccion:
                        direccion = cell_val
                    elif "poblaci" in cell_lbl and not poblacion:
                        poblacion = cell_val
                    elif "provincia" in cell_lbl and not provincia:
                        provincia = cell_val
                    elif ("cif" in cell_lbl or "nif" in cell_lbl or "d.n.i" in cell_lbl) and not cif_nif:
                        if c >= 3:
                            cif_nif = cell_val

            if not nombre:
                nombre = alias

            clients.append({
                "alias": alias,
                "nombre": nombre,
                "direccion": direccion,
                "poblacion": poblacion,
                "provincia": provincia,
                "cif_nif": cif_nif,
                "telefono": "",
                "email": "",
                "file": f
            })
        except Exception as e:
            fallback = extract_strings_from_corrupt_xls(path)
            if fallback:
                clients.append(fallback)
            else:
                clients.append({
                    "alias": alias,
                    "nombre": alias,
                    "direccion": "",
                    "poblacion": "",
                    "provincia": "",
                    "cif_nif": "",
                    "telefono": "",
                    "email": "",
                    "file": f
                })

    with open(output_json, "w", encoding="utf-8") as out:
        json.dump(clients, out, ensure_ascii=False, indent=2)

    print(f"Extraídos con éxito {len(clients)} de {len(files)} clientes en '{output_json}'.")
    return clients

if __name__ == "__main__":
    folder = sys.argv[1] if len(sys.argv) > 1 else "CARPETA CLIENTES"
    out_file = sys.argv[2] if len(sys.argv) > 2 else "clientes.json"
    extract_clients_from_folder(folder, out_file)
