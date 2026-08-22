#!/usr/bin/env python3
import os
import sys
import json
import re
import glob
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
    print(f"Extrayendo catálogo de tarifas desde: {precios_dir}")
    
    get_img = build_smart_image_lookup()
    products_map = {}

    # 1. ACCESORIOS.xlsx
    acc_path = os.path.join(precios_dir, "ACCESORIOS.xlsx")
    if os.path.exists(acc_path):
        wb = openpyxl.load_workbook(acc_path, data_only=True)
        for sname, sheet_name, default_cat in [
            ("ACCESORIO-ENRO PVP", "Accesorios", "Accesorios Persiana Enrollable"),
            ("ACCESORIO-COMP PVP", "Accesorios", "Accesorios Compactos")
        ]:
            if sname in wb.sheetnames:
                ws = wb[sname]
                cur_cat = default_cat
                for r in range(1, ws.max_row + 1):
                    c0 = str(ws.cell(r, 1).value or "").strip()
                    if "ACCESORIOS" in c0: cur_cat = c0
                    code = clean_code(ws.cell(r, 2).value)
                    desc = str(ws.cell(r, 3).value or "").strip()
                    pvp = clean_float(ws.cell(r, 4).value)
                    unit = str(ws.cell(r, 5).value or "").strip() or "ud."
                    if code and desc and pvp > 0:
                        products_map[code] = {
                            "sheet": sheet_name, "category": cur_cat, "code": code,
                            "desc": desc, "pvp": pvp, "t1": pvp, "u1": unit,
                            "p1": pvp, "p_t1": pvp, "img_path": get_img(code, desc, cur_cat)
                        }
        for sname in ["ACCESORIO-ENRO T-1", "ACCESORIO-COMP T-1"]:
            if sname in wb.sheetnames:
                ws = wb[sname]
                for r in range(1, ws.max_row + 1):
                    code = clean_code(ws.cell(r, 2).value)
                    t1 = clean_float(ws.cell(r, 4).value)
                    if code in products_map and t1 > 0:
                        products_map[code]["t1"] = t1
                        products_map[code]["p_t1"] = t1

    # 2. ENROLLABLES.xlsx
    enr_path = os.path.join(precios_dir, "ENROLLABLES.xlsx")
    if os.path.exists(enr_path):
        wb = openpyxl.load_workbook(enr_path, data_only=True)
        if "TARIFA PVP" in wb.sheetnames:
            ws = wb["TARIFA PVP"]
            cur_cat = "Persianas Enrollables"
            for r in range(1, ws.max_row + 1):
                c0 = str(ws.cell(r, 1).value or "").strip()
                c1 = str(ws.cell(r, 2).value or "").strip()
                if "PERSIANA" in c1 or "PERSIANA" in c0: cur_cat = c1 or c0
                code = clean_code(ws.cell(r, 2).value)
                desc = str(ws.cell(r, 3).value or "").strip()
                pvp = clean_float(ws.cell(r, 4).value)
                unit = str(ws.cell(r, 5).value or "").strip() or "m²"
                if code and code.isdigit() and desc and pvp > 0:
                    products_map[code] = {
                        "sheet": "Persianas Enrollables", "category": cur_cat, "code": code,
                        "desc": desc, "pvp": pvp, "t1": pvp, "u1": unit,
                        "p1": pvp, "p_t1": pvp, "img_path": get_img(code, desc, cur_cat)
                    }
        if "TARIFA T-1" in wb.sheetnames:
            ws = wb["TARIFA T-1"]
            for r in range(1, ws.max_row + 1):
                code = clean_code(ws.cell(r, 2).value)
                t1 = clean_float(ws.cell(r, 4).value)
                if code in products_map and t1 > 0:
                    products_map[code]["t1"] = t1
                    products_map[code]["p_t1"] = t1

    # 3. LAMAS Y CAJONES.xlsx
    lam_path = os.path.join(precios_dir, "LAMAS Y CAJONES.xlsx")
    if os.path.exists(lam_path):
        wb = openpyxl.load_workbook(lam_path, data_only=True)
        for pvp_sheet, t1_sheet, sheet_title, default_cat in [
            ("GUIAS y PERFILES PVP", "GUIAS y PERFILES T-1", "Guías y Perfiles", "Guías y Perfiles"),
            ("LAMAS y PERFILES PVP", "LAMAS y PERFILES T-1", "Lamas y Cajones", "Lamas y Perfiles")
        ]:
            if pvp_sheet in wb.sheetnames:
                ws = wb[pvp_sheet]
                cur_cat = default_cat
                for r in range(1, ws.max_row + 1):
                    c0 = str(ws.cell(r, 1).value or "").strip()
                    if "GUIAS" in c0 or "PERFILES" in c0 or "LAMAS" in c0: cur_cat = c0
                    code = clean_code(ws.cell(r, 2).value)
                    desc = str(ws.cell(r, 3).value or "").strip()
                    pvp = clean_float(ws.cell(r, 4).value)
                    unit = str(ws.cell(r, 5).value or "").strip() or "ml."
                    if (code and (code.isdigit() or "Tapón" in desc)) and desc and pvp > 0:
                        products_map[code] = {
                            "sheet": sheet_title, "category": cur_cat, "code": code,
                            "desc": desc, "pvp": pvp, "t1": pvp, "u1": unit,
                            "p1": pvp, "p_t1": pvp, "img_path": get_img(code, desc, cur_cat)
                        }
            if t1_sheet in wb.sheetnames:
                ws = wb[t1_sheet]
                for r in range(1, ws.max_row + 1):
                    code = clean_code(ws.cell(r, 2).value)
                    t1 = clean_float(ws.cell(r, 4).value)
                    if code in products_map and t1 > 0:
                        products_map[code]["t1"] = t1
                        products_map[code]["p_t1"] = t1

    # 4. MOTORES.xlsx
    mot_path = os.path.join(precios_dir, "MOTORES.xlsx")
    if os.path.exists(mot_path):
        wb = openpyxl.load_workbook(mot_path, data_only=True)
        if "MOTORES TARIFA PVP" in wb.sheetnames:
            ws = wb["MOTORES TARIFA PVP"]
            cur_cat = "Motores y Automatismos"
            for r in range(1, ws.max_row + 1):
                c0 = str(ws.cell(r, 1).value or "").strip()
                if "MOTORES" in c0: cur_cat = c0
                code = clean_code(ws.cell(r, 2).value)
                desc = str(ws.cell(r, 3).value or "").strip()
                pvp = clean_float(ws.cell(r, 4).value)
                unit = str(ws.cell(r, 5).value or "").strip() or "ud."
                if code and code.isdigit() and desc and pvp > 0:
                    products_map[code] = {
                        "sheet": "Motores y Automatismos", "category": cur_cat, "code": code,
                        "desc": desc, "pvp": pvp, "t1": pvp, "u1": unit,
                        "p1": pvp, "p_t1": pvp, "img_path": get_img(code, desc, cur_cat)
                    }
        if "MOTORES TARIFA T-1" in wb.sheetnames:
            ws = wb["MOTORES TARIFA T-1"]
            for r in range(1, ws.max_row + 1):
                code = clean_code(ws.cell(r, 2).value)
                t1 = clean_float(ws.cell(r, 4).value)
                if code in products_map and t1 > 0:
                    products_map[code]["t1"] = t1
                    products_map[code]["p_t1"] = t1

    # 5. VENECIANAS.xlsx
    ven_path = os.path.join(precios_dir, "VENECIANAS.xlsx")
    if os.path.exists(ven_path):
        wb = openpyxl.load_workbook(ven_path, data_only=True)
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

    # 6. FLEXOL.xlsx (Los Tejidos Técnicos)
    flx_path = os.path.join(precios_dir, "FLEXOL.xlsx")
    if os.path.exists(flx_path):
        wb = openpyxl.load_workbook(flx_path, data_only=True)
        ws = wb.active
        for r in range(2, ws.max_row + 1):
            code = clean_code(ws.cell(r, 1).value)
            category = str(ws.cell(r, 2).value or "Los Tejidos Técnicos").strip()
            desc = str(ws.cell(r, 3).value or "").strip()
            pvp = clean_float(ws.cell(r, 4).value)
            t1 = clean_float(ws.cell(r, 5).value)
            unit = str(ws.cell(r, 6).value or "m²").strip()
            if code and desc and pvp > 0:
                if t1 <= 0:
                    t1 = round(pvp * 0.75, 2)
                products_map[code] = {
                    "sheet": "Flexol",
                    "category": category,
                    "code": code,
                    "desc": desc,
                    "pvp": pvp,
                    "t1": t1,
                    "u1": unit,
                    "p1": pvp,
                    "p_t1": t1,
                    "img_path": get_img(code, desc, category)
                }

    # Conservar items personalizados si existen
    if os.path.exists(output_json):
        try:
            with open(output_json, "r", encoding="utf-8") as f_prev:
                prev_data = json.load(f_prev)
                for it in prev_data:
                    c = it.get("code")
                    if c and c not in products_map and it.get("is_custom"):
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

    print(f"Catálogo generado con {len(all_products)} artículos en '{output_json}' y 'precios_catalogo.xlsx'.")
    return all_products

if __name__ == "__main__":
    p_dir = sys.argv[1] if len(sys.argv) > 1 else "PRECIOS"
    out_f = sys.argv[2] if len(sys.argv) > 2 else "catalog.json"
    extract_all_catalog_precios(p_dir, out_f)
