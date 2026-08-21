#!/usr/bin/env python3
import os
import sys
import json
import re
import glob
import xlrd
import openpyxl
import fitz
import numpy as np
from PIL import Image as PILImage

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

def crop_cell_image(page, cell_bbox, out_path):
    if not cell_bbox:
        return None
    try:
        x0, y0, x1, y1 = cell_bbox
        rect = fitz.Rect(x0 + 1.0, y0 + 1.0, x1 - 1.0, y1 - 1.0)
        pix = page.get_pixmap(dpi=300, clip=rect)
        img = PILImage.frombytes("RGB", [pix.width, pix.height], pix.samples)

        arr = np.array(img)
        non_white = (arr[:, :, 0] < 240) | (arr[:, :, 1] < 240) | (arr[:, :, 2] < 240)
        rows = np.any(non_white, axis=1)
        cols = np.any(non_white, axis=0)

        if np.any(rows) and np.any(cols):
            ymin, ymax = np.where(rows)[0][[0, -1]]
            xmin, xmax = np.where(cols)[0][[0, -1]]
            pad = 6
            ymin = max(0, ymin - pad)
            ymax = min(arr.shape[0], ymax + pad)
            xmin = max(0, xmin - pad)
            xmax = min(arr.shape[1], xmax + pad)
            cropped_arr = arr[ymin:ymax, xmin:xmax]
            img = PILImage.fromarray(cropped_arr)
        else:
            return None

        img.save(out_path, format="PNG")
        return out_path
    except Exception:
        return None

def build_smart_image_lookup():
    all_imgs = sorted(glob.glob(os.path.join(OUTPUT_DIR, "*.png")))
    
    def get_image(code, desc, category=""):
        # 1. Exact code match in filename
        if code:
            for img in all_imgs:
                base = os.path.basename(img)
                if code in base:
                    return img
        # 2. Prefix 6-7 digit match
        if code and len(code) >= 6:
            prefix = code[:6]
            for img in all_imgs:
                base = os.path.basename(img)
                if prefix in base:
                    return img
        # 3. Description keyword fallback
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
        return ""

    return get_image

def extract_all_catalog_precios(precios_dir="PRECIOS", output_json="catalog.json"):
    print(f"Extrayendo catálogo PVP desde: {precios_dir}")
    
    # 1. Extraer imágenes de PDFs en PRECIOS
    acc_pdf = os.path.join(precios_dir, "ACCESORIOS ENERO 2026.pdf")
    if os.path.exists(acc_pdf):
        try:
            doc_acc = fitz.open(acc_pdf)
            for pno in range(min(4, len(doc_acc))):
                page = doc_acc[pno]
                tabs = page.find_tables()
                if not tabs.tables:
                    continue
                tab = tabs.tables[0]
                ext = tab.extract()
                for r_idx, r in enumerate(tab.rows):
                    if r_idx == 0:
                        continue
                    row_vals = ext[r_idx]
                    code_w = clean_code(row_vals[1] if len(row_vals) > 1 else "")
                    if code_w and r.cells[0]:
                        out = os.path.join(OUTPUT_DIR, f"acc2026_p{pno+1}_{code_w}.png")
                        crop_cell_image(page, r.cells[0], out)
        except Exception as e:
            print(f"Aviso al procesar imágenes de {acc_pdf}: {e}")

    enr_pdf = os.path.join(precios_dir, "ENROLLABLES ENERO 2026.pdf")
    if os.path.exists(enr_pdf):
        try:
            doc_enr = fitz.open(enr_pdf)
            page_enr = doc_enr[0]
            tabs_enr = page_enr.find_tables()
            for t_idx, tab in enumerate(tabs_enr.tables):
                ext = tab.extract()
                for r_idx, r in enumerate(tab.rows):
                    row_vals = ext[r_idx]
                    code_w = clean_code(row_vals[1] if len(row_vals) > 1 else "")
                    if code_w and code_w.isdigit() and r.cells[0]:
                        out = os.path.join(OUTPUT_DIR, f"enr2026_{code_w}.png")
                        crop_cell_image(page_enr, r.cells[0], out)
        except Exception as e:
            print(f"Aviso al procesar imágenes de {enr_pdf}: {e}")

    lam_pdf = os.path.join(precios_dir, "LAMAS Y CAJONES 2026.pdf")
    if os.path.exists(lam_pdf):
        try:
            doc_lam = fitz.open(lam_pdf)
            page_lam = doc_lam[0]
            tabs_lam = page_lam.find_tables()
            for t_idx, tab in enumerate(tabs_lam.tables):
                ext = tab.extract()
                for r_idx, r in enumerate(tab.rows):
                    row_vals = ext[r_idx]
                    code_w = clean_code(row_vals[1] if len(row_vals) > 1 else "")
                    if code_w and code_w.isdigit() and r.cells[0]:
                        out = os.path.join(OUTPUT_DIR, f"lam2026_t{t_idx}_{code_w}.png")
                        crop_cell_image(page_lam, r.cells[0], out)
        except Exception as e:
            print(f"Aviso al procesar imágenes de {lam_pdf}: {e}")

    get_img = build_smart_image_lookup()
    all_products = []

    # ----------------------------------------------------
    # 1. ACCESORIOS ENERO 2026.xls
    # ----------------------------------------------------
    acc_xls = os.path.join(precios_dir, "ACCESORIOS ENERO 2026.xls")
    if os.path.exists(acc_xls):
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
                    c1 = sh.cell_value(r, 1)
                    c2 = str(sh.cell_value(r, 2)).strip()
                    c3 = sh.cell_value(r, 3)
                    c4 = sh.cell_value(r, 4)
                    c5 = sh.cell_value(r, 5)
                    c6 = sh.cell_value(r, 6)

                    if "ACCESORIOS" in c0:
                        cur_cat = c0

                    code = clean_code(c1)
                    pvp = clean_float(c3)
                    unit = str(c4).strip() if c4 else "ud."
                    pvp_iva = clean_float(c5)
                    unit2 = str(c6).strip() if c6 else unit

                    if code and c2 and pvp > 0:
                        img_path = get_img(code, c2, cur_cat)
                        all_products.append({
                            "sheet": sheet_name,
                            "category": cur_cat,
                            "code": code,
                            "desc": c2,
                            "p1": pvp,
                            "u1": unit,
                            "p2": pvp_iva if pvp_iva > 0 else round(pvp * 1.21, 2),
                            "u2": unit2,
                            "img_path": img_path
                        })

    # ----------------------------------------------------
    # 2. ENROLLABLES ENERO 2026.xls
    # ----------------------------------------------------
    enr_xls = os.path.join(precios_dir, "ENROLLABLES ENERO 2026.xls")
    if os.path.exists(enr_xls):
        wb = xlrd.open_workbook(enr_xls)
        if "TARIFA PVP" in wb.sheet_names():
            sh = wb.sheet_by_name("TARIFA PVP")
            cur_cat = "Persianas Enrollables"
            for r in range(sh.nrows):
                c0 = str(sh.cell_value(r, 0)).strip()
                c1 = str(sh.cell_value(r, 1)).strip()
                c2 = str(sh.cell_value(r, 2)).strip()
                c3 = sh.cell_value(r, 3)
                c4 = sh.cell_value(r, 4)
                c5 = sh.cell_value(r, 5)
                c6 = sh.cell_value(r, 6)

                if "PERSIANA" in c1 or "PERSIANA" in c0:
                    cur_cat = c1 if c1 else c0

                code = clean_code(sh.cell_value(r, 1))
                pvp = clean_float(c3)
                unit = str(c4).strip() if c4 else "m²"
                pvp_iva = clean_float(c5)
                unit2 = str(c6).strip() if c6 else unit

                if code and code.isdigit() and c2 and pvp > 0:
                    img_path = get_img(code, c2, cur_cat)
                    all_products.append({
                        "sheet": "Persianas Enrollables",
                        "category": cur_cat,
                        "code": code,
                        "desc": c2,
                        "p1": pvp,
                        "u1": unit,
                        "p2": pvp_iva if pvp_iva > 0 else round(pvp * 1.21, 2),
                        "u2": unit2,
                        "img_path": img_path
                    })

    # ----------------------------------------------------
    # 3. LAMAS Y CAJONES 2026.xls
    # ----------------------------------------------------
    lam_xls = os.path.join(precios_dir, "LAMAS Y CAJONES 2026.xls")
    if os.path.exists(lam_xls):
        wb = xlrd.open_workbook(lam_xls)
        # GUIAS y PERFILES PVP
        if "GUIAS y PERFILES PVP" in wb.sheet_names():
            sh = wb.sheet_by_name("GUIAS y PERFILES PVP")
            cur_cat = "Guías y Perfiles"
            for r in range(sh.nrows):
                c0 = str(sh.cell_value(r, 0)).strip()
                c1 = sh.cell_value(r, 1)
                c2 = str(sh.cell_value(r, 2)).strip()
                c3 = sh.cell_value(r, 3)
                c4 = sh.cell_value(r, 4)
                c5 = sh.cell_value(r, 5)
                c6 = sh.cell_value(r, 6)

                if "GUIAS" in c0 or "PERFILES" in c0:
                    cur_cat = c0

                code = clean_code(c1)
                pvp = clean_float(c3)
                unit = str(c4).strip() if c4 else "ml."
                pvp_iva = clean_float(c5)
                unit2 = str(c6).strip() if c6 else unit

                if (code and code.isdigit() or "Tapón" in c2) and c2 and pvp > 0:
                    img_path = get_img(code, c2, cur_cat)
                    all_products.append({
                        "sheet": "Guías y Perfiles",
                        "category": cur_cat,
                        "code": code,
                        "desc": c2,
                        "p1": pvp,
                        "u1": unit,
                        "p2": pvp_iva if pvp_iva > 0 else round(pvp * 1.21, 2),
                        "u2": unit2,
                        "img_path": img_path
                    })

        # LAMAS y PERFILES PVP
        if "LAMAS y PERFILES PVP" in wb.sheet_names():
            sh = wb.sheet_by_name("LAMAS y PERFILES PVP")
            cur_cat = "Lamas y Perfiles"
            for r in range(sh.nrows):
                c0 = str(sh.cell_value(r, 0)).strip()
                c1 = sh.cell_value(r, 1)
                c2 = str(sh.cell_value(r, 2)).strip()
                c3 = sh.cell_value(r, 3)
                c4 = sh.cell_value(r, 4)
                c5 = sh.cell_value(r, 5)
                c6 = sh.cell_value(r, 6)

                if "LAMAS" in c0 or "PERFILES" in c0:
                    cur_cat = c0

                code = clean_code(c1)
                pvp = clean_float(c3)
                unit = str(c4).strip() if c4 else "ml."
                pvp_iva = clean_float(c5)
                unit2 = str(c6).strip() if c6 else unit

                if code and code.isdigit() and c2 and pvp > 0:
                    img_path = get_img(code, c2, cur_cat)
                    all_products.append({
                        "sheet": "Lamas y Cajones",
                        "category": cur_cat,
                        "code": code,
                        "desc": c2,
                        "p1": pvp,
                        "u1": unit,
                        "p2": pvp_iva if pvp_iva > 0 else round(pvp * 1.21, 2),
                        "u2": unit2,
                        "img_path": img_path
                    })

    # ----------------------------------------------------
    # 4. TARIFAS MOTORES ABRIL 2024.xls
    # ----------------------------------------------------
    mot_xls = os.path.join(precios_dir, "TARIFAS MOTORES ABRIL 2024.xls")
    if os.path.exists(mot_xls):
        wb = xlrd.open_workbook(mot_xls)
        if "MOTORES TARIFA PVP" in wb.sheet_names():
            sh = wb.sheet_by_name("MOTORES TARIFA PVP")
            cur_cat = "Motores y Automatismos"
            for r in range(sh.nrows):
                c0 = str(sh.cell_value(r, 0)).strip()
                c1 = sh.cell_value(r, 1)
                c2 = str(sh.cell_value(r, 2)).strip()
                c3 = sh.cell_value(r, 3)
                c4 = sh.cell_value(r, 4)
                c5 = sh.cell_value(r, 5)
                c6 = sh.cell_value(r, 6)

                if "MOTORES" in c0:
                    cur_cat = c0

                code = clean_code(c1)
                pvp = clean_float(c3)
                unit = str(c4).strip() if c4 else "ud."
                pvp_iva = clean_float(c5)
                unit2 = str(c6).strip() if c6 else unit

                if code and code.isdigit() and c2 and pvp > 0:
                    img_path = get_img(code, c2, cur_cat)
                    all_products.append({
                        "sheet": "Motores y Automatismos",
                        "category": cur_cat,
                        "code": code,
                        "desc": c2,
                        "p1": pvp,
                        "u1": unit,
                        "p2": pvp_iva if pvp_iva > 0 else round(pvp * 1.21, 2),
                        "u2": unit2,
                        "img_path": img_path
                    })

    # ----------------------------------------------------
    # 5. VENECIANAS ENERO 2026.xlsx
    # ----------------------------------------------------
    ven_xlsx = os.path.join(precios_dir, "VENECIANAS ENERO 2026.xlsx")
    if os.path.exists(ven_xlsx):
        wb = openpyxl.load_workbook(ven_xlsx, data_only=True)
        ws = wb.active
        v_idx = 1
        for r in range(1, ws.max_row + 1):
            val = ws.cell(r, 1).value
            if val:
                val_str = str(val).strip()
                val_clean = val_str.replace("45 00 €", "45,00 €")
                m = re.search(r"^(.*?)\s+([0-9]+(?:[\.,][0-9]+)?)\s*€", val_clean)
                if m:
                    desc = m.group(1).strip()
                    pvp = float(m.group(2).replace(",", "."))
                    unit = "m²" if "GRADUABLE" in desc else ("ml." if "CORDON" in desc or "ESCALERILLA" in desc else "ud.")
                    pvp_iva = round(pvp * 1.21, 2)
                    code = f"VEN{v_idx:03d}"
                    v_idx += 1
                    img_path = get_img(code, desc, "Venecianas")
                    all_products.append({
                        "sheet": "Venecianas y Graduables",
                        "category": "Venecianas",
                        "code": code,
                        "desc": desc,
                        "p1": pvp,
                        "u1": unit,
                        "p2": pvp_iva,
                        "u2": unit,
                        "img_path": img_path
                    })

    with open(output_json, "w", encoding="utf-8") as out:
        json.dump(all_products, out, ensure_ascii=False, indent=2)

    print(f"Generado con éxito '{output_json}' con {len(all_products)} artículos en PVP.")
    return all_products

if __name__ == "__main__":
    p_dir = sys.argv[1] if len(sys.argv) > 1 else "PRECIOS"
    out_f = sys.argv[2] if len(sys.argv) > 2 else "catalog.json"
    extract_all_catalog_precios(p_dir, out_f)
