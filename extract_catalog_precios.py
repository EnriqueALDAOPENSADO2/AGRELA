#!/usr/bin/env python3
import os
import sys
import json
import re
import glob
import xlrd
import openpyxl

OUTPUT_DIR = "extracted_images"
os.makedirs(OUTPUT_DIR, exist_ok=True)

def clean_code(val):
    if val is None or val == "":
        return ""
    if isinstance(val, float):
        if val.is_integer():
            return str(int(val))
    s = str(val).strip()
    if s.endswith(".0") and s[:-2].isdigit():
        return s[:-2]
    return s

def clean_float(val):
    if val is None or val == "":
        return 0.0
    if isinstance(val, (int, float)):
        return float(val)
    s = str(val).strip().replace("€", "").replace(" ", "")
    s = s.replace(",", ".")
    try:
        return float(s)
    except ValueError:
        return 0.0

def build_smart_image_lookup():
    all_imgs = sorted(glob.glob(os.path.join(OUTPUT_DIR, "*.png")))
    
    def get_image(code, desc, category=""):
        if code:
            for img in all_imgs:
                base = os.path.basename(img)
                if code in base:
                    return img
        if code and len(code) >= 6:
            prefix = code[:6]
            for img in all_imgs:
                base = os.path.basename(img)
                if prefix in base:
                    return img
        d = desc.lower()
        if "cajon blanco" in d:
            return "extracted_images/compactos_290155000.png"
        if "lama 43" in d or "aluminio de 43" in d:
            return "extracted_images/lam2026_t0_280043008.png"
        if "pvc r - 45" in d or "pvc radial 45" in d:
            return "extracted_images/lam2026_t1_220010001.png"
        if "pvc mini" in d:
            return "extracted_images/lam2026_t2_260010001.png"
        if "bajera de 44" in d:
            return "extracted_images/lam2026_t3_280000015.png"
        if "bajera de 53" in d:
            return "extracted_images/lam2026_t4_280000019.png"
        if "juego de perfiles 155" in d:
            return "extracted_images/lam2026_t5_290000001.png"
        if "juego de perfiles 185" in d:
            return "extracted_images/lam2026_t5_290000004.png"
        if "perfil lateral" in d:
            return "extracted_images/lam2026_t6_290000007.png"
        if "perfil fondo" in d:
            return "extracted_images/lam2026_t7_290000011.png"
        if "autoblocante" in d:
            return "extracted_images/compactos_290155007.png"
        if "motor" in d:
            return "extracted_images/motor_p1_600500001.png"
        if "mosquitera" in d or "mosquiflex" in d:
            return "extracted_images/motor_p1_600500037.png"
        return ""

    return get_image

def extract_all_catalog_precios(precios_dir="PRECIOS", output_json="catalog.json"):
    print(f"Extrayendo catálogo de precios 2026 desde: {precios_dir}")
    
    get_img = build_smart_image_lookup()
    products_map = {}

    # 1. ACCESORIOS ENERO 2026.xls
    acc_files = [os.path.join(precios_dir, "ACCESORIOS ENERO 2026.xls"), os.path.join(precios_dir, "accesorios.xls")]
    acc_xls = next((f for f in acc_files if os.path.exists(f)), None)
    if acc_xls:
        wb = xlrd.open_workbook(acc_xls)
        for sname, sheet_name, default_cat in [
            ("ACCESORIO-ENRO PVP", "Accesorios", "Accesorios Persiana Enrollable"),
            ("ACCESORIO-COMP PVP", "Accesorios", "Accesorios Compactos")
        ]:
            if sname in wb.sheet_names():
                sh = wb.sheet_by_name(sname)
                cur_cat = default_cat
                for r in range(sh.nrows):
                    c0 = str(sh.cell_value(r, 0)).strip()
                    if "ACCESORIOS" in c0: cur_cat = c0
                    code = clean_code(sh.cell_value(r, 1))
                    desc = str(sh.cell_value(r, 2)).strip()
                    pvp = clean_float(sh.cell_value(r, 3))
                    unit = str(sh.cell_value(r, 4)).strip() or "ud."
                    if code and desc and pvp > 0:
                        products_map[code] = {
                            "sheet": sheet_name, "category": cur_cat, "code": code,
                            "desc": desc, "pvp": pvp, "t1": pvp, "u1": unit,
                            "p1": pvp, "p_t1": pvp, "img_path": get_img(code, desc, cur_cat)
                        }
        for sname in ["ACCESORIO-ENRO T-1", "ACCESORIO-COMP T-1"]:
            if sname in wb.sheet_names():
                sh = wb.sheet_by_name(sname)
                for r in range(sh.nrows):
                    code = clean_code(sh.cell_value(r, 1))
                    t1 = clean_float(sh.cell_value(r, 3))
                    if code in products_map and t1 > 0:
                        products_map[code]["t1"] = t1
                        products_map[code]["p_t1"] = t1

    # 2. ENROLLABLES ENERO 2026.xls
    enr_files = [os.path.join(precios_dir, "ENROLLABLES ENERO 2026.xls"), os.path.join(precios_dir, "enrollables.xls")]
    enr_xls = next((f for f in enr_files if os.path.exists(f)), None)
    if enr_xls:
        wb = xlrd.open_workbook(enr_xls)
        if "TARIFA PVP" in wb.sheet_names():
            sh = wb.sheet_by_name("TARIFA PVP")
            cur_cat = "Persianas Enrollables"
            for r in range(sh.nrows):
                c0 = str(sh.cell_value(r, 0)).strip()
                c1 = str(sh.cell_value(r, 1)).strip()
                if "PERSIANA" in c1 or "PERSIANA" in c0: cur_cat = c1 or c0
                code = clean_code(sh.cell_value(r, 1))
                desc = str(sh.cell_value(r, 2)).strip()
                pvp = clean_float(sh.cell_value(r, 3))
                unit = str(sh.cell_value(r, 4)).strip() or "m²"
                if code and code.isdigit() and desc and pvp > 0:
                    products_map[code] = {
                        "sheet": "Persianas Enrollables", "category": cur_cat, "code": code,
                        "desc": desc, "pvp": pvp, "t1": pvp, "u1": unit,
                        "p1": pvp, "p_t1": pvp, "img_path": get_img(code, desc, cur_cat)
                    }
        if "TARIFA T-1" in wb.sheet_names():
            sh = wb.sheet_by_name("TARIFA T-1")
            for r in range(sh.nrows):
                code = clean_code(sh.cell_value(r, 1))
                t1 = clean_float(sh.cell_value(r, 3))
                if code in products_map and t1 > 0:
                    products_map[code]["t1"] = t1
                    products_map[code]["p_t1"] = t1

    # 3. LAMAS Y CAJONES 2026.xls
    lam_files = [os.path.join(precios_dir, "LAMAS Y CAJONES 2026.xls"), os.path.join(precios_dir, "lamas_y_cajones.xls")]
    lam_xls = next((f for f in lam_files if os.path.exists(f)), None)
    if lam_xls:
        wb = xlrd.open_workbook(lam_xls)
        for pvp_sheet, t1_sheet, sheet_title, default_cat in [
            ("GUIAS y PERFILES PVP", "GUIAS y PERFILES T-1", "Guías y Perfiles", "Guías y Perfiles"),
            ("LAMAS y PERFILES PVP", "LAMAS y PERFILES T-1", "Lamas y Cajones", "Lamas y Perfiles")
        ]:
            if pvp_sheet in wb.sheet_names():
                sh = wb.sheet_by_name(pvp_sheet)
                cur_cat = default_cat
                for r in range(sh.nrows):
                    c0 = str(sh.cell_value(r, 0)).strip()
                    if "GUIAS" in c0 or "PERFILES" in c0 or "LAMAS" in c0: cur_cat = c0
                    code = clean_code(sh.cell_value(r, 1))
                    desc = str(sh.cell_value(r, 2)).strip()
                    pvp = clean_float(sh.cell_value(r, 3))
                    unit = str(sh.cell_value(r, 4)).strip() or "ml."
                    if (code and (code.isdigit() or "Tapón" in desc)) and desc and pvp > 0:
                        products_map[code] = {
                            "sheet": sheet_title, "category": cur_cat, "code": code,
                            "desc": desc, "pvp": pvp, "t1": pvp, "u1": unit,
                            "p1": pvp, "p_t1": pvp, "img_path": get_img(code, desc, cur_cat)
                        }
            if t1_sheet in wb.sheet_names():
                sh = wb.sheet_by_name(t1_sheet)
                for r in range(sh.nrows):
                    code = clean_code(sh.cell_value(r, 1))
                    t1 = clean_float(sh.cell_value(r, 3))
                    if code in products_map and t1 > 0:
                        products_map[code]["t1"] = t1
                        products_map[code]["p_t1"] = t1

    # 4. TARIFAS MOTORES ENERO 2026.xls
    mot_files = [os.path.join(precios_dir, "TARIFAS MOTORES ENERO 2026.xls"), os.path.join(precios_dir, "motores.xls")]
    mot_xls = next((f for f in mot_files if os.path.exists(f)), None)
    if mot_xls:
        wb = xlrd.open_workbook(mot_xls)
        if "MOTORES TARIFA PVP" in wb.sheet_names():
            sh = wb.sheet_by_name("MOTORES TARIFA PVP")
            cur_cat = "Motores y Automatismos"
            for r in range(sh.nrows):
                c0 = str(sh.cell_value(r, 0)).strip()
                if "MOTORES" in c0: cur_cat = c0
                code = clean_code(sh.cell_value(r, 1))
                desc = str(sh.cell_value(r, 2)).strip()
                pvp = clean_float(sh.cell_value(r, 3))
                unit = str(sh.cell_value(r, 4)).strip() or "ud."
                if code and code.isdigit() and desc and pvp > 0:
                    products_map[code] = {
                        "sheet": "Motores y Automatismos", "category": cur_cat, "code": code,
                        "desc": desc, "pvp": pvp, "t1": pvp, "u1": unit,
                        "p1": pvp, "p_t1": pvp, "img_path": get_img(code, desc, cur_cat)
                    }
        if "MOTORES TARIFA T-1" in wb.sheet_names():
            sh = wb.sheet_by_name("MOTORES TARIFA T-1")
            for r in range(sh.nrows):
                code = clean_code(sh.cell_value(r, 1))
                t1 = clean_float(sh.cell_value(r, 3))
                if code in products_map and t1 > 0:
                    products_map[code]["t1"] = t1
                    products_map[code]["p_t1"] = t1

    # 5. VENECIANAS ENERO 2026.xlsx
    ven_files = [os.path.join(precios_dir, "VENECIANAS ENERO 2026.xlsx"), os.path.join(precios_dir, "venecianas.xlsx")]
    ven_xlsx = next((f for f in ven_files if os.path.exists(f)), None)
    if ven_xlsx:
        wb = openpyxl.load_workbook(ven_xlsx, data_only=True)
        ws = wb.active
        v_idx = 1
        for r in range(1, ws.max_row + 1):
            val = ws.cell(r, 1).value
            if val:
                val_str = str(val).strip().replace("45 00 €", "45,00 €")
                m = re.search(r"^(.*?)\s+([0-9]+(?:[\.,][0-9]+)?)\s*€", val_str)
                if m:
                    desc = m.group(1).strip()
                    pvp = float(m.group(2).replace(",", "."))
                    t1 = round(pvp * 0.75, 2)
                    unit = "m²" if "GRADUABLE" in desc else ("ml." if "CORDON" in desc or "ESCALERILLA" in desc else "ud.")
                    code = f"VEN{v_idx:03d}"
                    v_idx += 1
                    products_map[code] = {
                        "sheet": "Venecianas y Graduables", "category": "Venecianas", "code": code,
                        "desc": desc, "pvp": pvp, "t1": t1, "u1": unit,
                        "p1": pvp, "p_t1": t1, "img_path": get_img(code, desc, "Venecianas")
                    }

    # Si hay productos personalizados añadidos previamente en precios_catalogo.xlsx o catalog.json, conservarlos
    if os.path.exists(output_json):
        try:
            with open(output_json, "r", encoding="utf-8") as f_prev:
                prev_data = json.load(f_prev)
                for it in prev_data:
                    c = it.get("code")
                    if c and c not in products_map and (it.get("is_custom") or it.get("sheet") in ["Flexol", "Mosquiflex", "Personalizada", "Varios"]):
                        products_map[c] = it
        except Exception:
            pass

    all_products = list(products_map.values())
    with open(output_json, "w", encoding="utf-8") as out:
        json.dump(all_products, out, ensure_ascii=False, indent=2)

    # Generar precios_catalogo.xlsx
    wb_cat = openpyxl.Workbook()
    ws_cat = wb_cat.active
    ws_cat.title = "Precios"
    ws_cat.append(["Código", "Descripción", "PVP (€)", "Tarifa 1 (€)", "Unidad", "Categoría / Hoja", "Croquis Imagen"])

    for it in all_products:
        ws_cat.append([
            it.get("code", ""),
            it.get("desc", ""),
            it.get("pvp", it.get("p1", 0.0)),
            it.get("t1", it.get("p_t1", 0.0)),
            it.get("u1", "ud."),
            it.get("sheet", ""),
            it.get("img_path", "")
        ])

    wb_cat.save("precios_catalogo.xlsx")
    wb_cat.save("windows/precios_catalogo.xlsx")
    with open("windows/catalog.json", "w", encoding="utf-8") as out_win:
        json.dump(all_products, out_win, ensure_ascii=False, indent=2)

    print(f"Catálogo 2026 generado con {len(all_products)} artículos en '{output_json}' y 'precios_catalogo.xlsx'.")
    return all_products

if __name__ == "__main__":
    p_dir = sys.argv[1] if len(sys.argv) > 1 else "PRECIOS"
    out_f = sys.argv[2] if len(sys.argv) > 2 else "catalog.json"
    extract_all_catalog_precios(p_dir, out_f)
