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

def extract_mosquiflex(pdf_path):
    items = []
    if not os.path.exists(pdf_path):
        return items

    models = [
        ("MQ-ENR-01", "Mosquitera Enrollable Ventana 42 - Grupo 1 (Blanco)", 48.0, "ud.", "Enrollables Ventana"),
        ("MQ-ENR-02", "Mosquitera Enrollable Ventana 42 - Grupo 2 (Plata/Bronce/RAL)", 54.0, "ud.", "Enrollables Ventana"),
        ("MQ-ENR-03", "Mosquitera Enrollable Ventana 42 - Grupo 3 (Madera)", 62.0, "ud.", "Enrollables Ventana"),
        ("MQ-ENR-PTA1", "Mosquitera Enrollable Puerta Lateral Única - Grupo 1", 125.0, "ud.", "Enrollable Puerta"),
        ("MQ-ENR-PTA2", "Mosquitera Enrollable Puerta Lateral Doble - Grupo 1", 230.0, "ud.", "Enrollable Puerta"),
        ("MQ-PLI-22-1", "Mosquitera Plisada 22 Puerta Única - Grupo 1", 135.0, "ud.", "Plisada 22"),
        ("MQ-PLI-22-2", "Mosquitera Plisada 22 Puerta Doble - Grupo 1", 245.0, "ud.", "Plisada 22"),
        ("MQ-PLI-22-REV", "Mosquitera Plisada 22 Reversible - Grupo 1", 155.0, "ud.", "Plisada 22"),
        ("MQ-PLI-22-VEN", "Mosquitera Plisada 22 Ventana - Grupo 1", 85.0, "ud.", "Plisada 22"),
        ("MQ-PLI-40-1", "Mosquitera Plisada 40 Puerta Única - Grupo 1", 326.0, "ud.", "Plisada 40"),
        ("MQ-PLI-40-2", "Mosquitera Plisada 40 Puerta Doble - Grupo 1", 653.0, "ud.", "Plisada 40"),
        ("MQ-ABA-1", "Mosquitera Abatible Puerta 1 Hoja - Grupo 1", 303.0, "ud.", "Abatible Puerta"),
        ("MQ-ABA-2", "Mosquitera Abatible Puerta 2 Hojas - Grupo 1", 605.0, "ud.", "Abatible Puerta"),
        ("MQ-FIJ-1", "Mosquitera Fija con Marco - Grupo 1", 38.0, "m²", "Mosquitera Fija"),
        ("MQ-COR-1", "Mosquitera Corredera Perfil Curvo - Grupo 1", 45.0, "m²", "Mosquitera Corredera"),
        ("MQ-EXP-01", "Carta de Colores Mosquiflex", 10.0, "ud.", "Muestras y Expositores"),
        ("MQ-EXP-02", "Expositor Personalizable Mosquiflex", 590.0, "ud.", "Muestras y Expositores")
    ]
    for code, desc, price, unit, cat in models:
        items.append({
            "sheet": "Mosquiflex", "category": cat, "code": code, "desc": desc,
            "pvp": price, "t1": round(price * 0.75, 2), "u1": unit,
            "p1": price, "p_t1": round(price * 0.75, 2), "img_path": ""
        })

    try:
        doc = fitz.open(pdf_path)
        for pno in range(len(doc)):
            page = doc[pno]
            tabs = page.find_tables()
            for tab in tabs.tables:
                ext = tab.extract()
                for row in ext:
                    for cell in row:
                        if not cell: continue
                        lines = [l.strip() for l in cell.split('\n') if l.strip()]
                        for i, l in enumerate(lines):
                            m = re.search(r'([0-9]{1,3}[,\.][0-9]{2})\s*€?', l)
                            if m and i > 0:
                                desc = ' '.join(lines[:i]).strip()
                                if len(desc) > 3 and not desc.startswith('G1') and not desc.startswith('G2') and not desc.startswith('Gr'):
                                    try:
                                        p = float(m.group(1).replace(',', '.'))
                                        if 0.05 <= p <= 500:
                                            code = f'MQ-{pno+1:02d}-{len(items)+1:03d}'
                                            items.append({
                                                "sheet": "Mosquiflex", "category": f"Accesorios Pág. {pno+1}",
                                                "code": code, "desc": desc, "pvp": p, "t1": round(p * 0.75, 2),
                                                "u1": "ml." if any(w in desc.lower() for w in ['perfil', 'carril', 'guía', 'burlete']) else "ud.",
                                                "p1": p, "p_t1": round(p * 0.75, 2), "img_path": ""
                                            })
                                    except: pass
    except Exception as e:
        print(f"Aviso parseando Mosquiflex: {e}")

    return items

def extract_flexol(pdf_path):
    items = []
    if not os.path.exists(pdf_path):
        return items

    models = [
        ("FLX-VEN-16", "Veneciana Aluminio 16 mm - Colores Básicos", 35.0, "m²", "Venecianas"),
        ("FLX-VEN-25", "Veneciana Aluminio 25 mm - Colores Básicos", 29.0, "m²", "Venecianas"),
        ("FLX-VEN-50", "Veneciana Aluminio 50 mm - Colores Básicos con Cordón", 42.0, "m²", "Venecianas"),
        ("FLX-VEN-50C", "Veneciana Aluminio 50 mm - Colores Básicos con Cinta", 48.0, "m²", "Venecianas"),
        ("FLX-VEN-MAD", "Veneciana Madera 50 mm - Colección Bali / Madeira", 78.0, "m²", "Venecianas Madera"),
        ("FLX-VT-89", "Cortina Vertical 89 mm - Colección Teide / Alcazaba", 42.0, "m²", "Cortinas Verticales"),
        ("FLX-VT-127", "Cortina Vertical 127 mm - Colección Teide / Alcazaba", 38.0, "m²", "Cortinas Verticales"),
        ("FLX-PLI-01", "Cortina Plisada Premium PLI.01 - Colección 236", 58.0, "m²", "Cortinas Plisadas"),
        ("FLX-PLI-REV", "Cortina Plisada Repliegue Reversible PLI.02", 68.0, "m²", "Cortinas Plisadas"),
        ("FLX-PLI-ND", "Cortina Plisada Noche y Día PLI.03", 85.0, "m²", "Cortinas Plisadas"),
        ("FLX-PLI-MINI", "Cortina Plisada Mini Cristal Accionamiento Manual", 49.0, "m²", "Cortinas Plisadas"),
        ("FLX-PLE-01", "Cortina Plegable Confeccionada Sistema Plus", 55.0, "m²", "Cortinas Plegables"),
        ("FLX-PAN-01", "Panel Deslizante / Japonés - Sistema Básico 3 Vías", 65.0, "m²", "Paneles Deslizantes"),
        ("FLX-PAN-02", "Panel Deslizante / Japonés - Sistema Motor 4 Vías", 120.0, "m²", "Paneles Deslizantes"),
        ("FLX-ENR-UNI", "Cortina Enrollable Universal EN.0 (Sin Tejido)", 28.0, "ud.", "Enrollables Flexol"),
        ("FLX-ENR-SAT", "Cortina Enrollable Screen Satiné 5500", 63.0, "m²", "Enrollables Flexol"),
        ("FLX-ENR-CAJ42", "Cortina Enrollable con Cajón de 42 mm", 72.0, "m²", "Enrollables con Cajón"),
        ("FLX-ENR-CAJ90", "Cortina Enrollable con Cajón de 90 mm", 98.0, "m²", "Enrollables con Cajón"),
        ("FLX-ENR-ZIP", "Cortina Enrollable Sistema ZIP Guiada", 135.0, "m²", "Enrollables Guiadas ZIP"),
        ("FLX-LM-01", "Cortina Luz Mágica LM.01 Alborada Galería 80", 88.0, "m²", "Cortinas Luz Mágica"),
        ("FLX-LM-02", "Cortina Luz Mágica LM.03 Niebla Galería 90", 96.0, "m²", "Cortinas Luz Mágica")
    ]
    for code, desc, price, unit, cat in models:
        items.append({
            "sheet": "Flexol", "category": cat, "code": code, "desc": desc,
            "pvp": price, "t1": round(price * 0.75, 2), "u1": unit,
            "p1": price, "p_t1": round(price * 0.75, 2), "img_path": ""
        })

    try:
        doc = fitz.open(pdf_path)
        for pno in range(len(doc)):
            page = doc[pno]
            text = page.get_text()
            if any(k in text.upper() for k in ['DENOMINACIÓN', 'COMPONENTES', 'INCREMENTOS', '€ / UD', '€/UD']):
                tabs = page.find_tables()
                for tab in tabs.tables:
                    ext = tab.extract()
                    for row in ext:
                        if not row: continue
                        r_str = ' | '.join([str(c) for c in row if c])
                        m = re.search(r'([A-Za-zÁÉÍÓÚáéíóúñÑ0-9\s\/\-\.\(\)\:\,]{4,50})\s+([0-9]{1,3}[,\.][0-9]{2})', r_str)
                        if m:
                            desc = m.group(1).strip()
                            if len(desc) > 3 and not desc.upper().startswith('PÁG') and not desc.upper().startswith('DENOM'):
                                try:
                                    p = float(m.group(2).replace(',', '.'))
                                    if 0.05 <= p <= 500:
                                        code = f'FLX-{pno+1:03d}-{len(items)+1:03d}'
                                        items.append({
                                            "sheet": "Flexol", "category": f"Componentes (Pág. {pno+1})",
                                            "code": code, "desc": desc, "pvp": p, "t1": round(p * 0.75, 2),
                                            "u1": "ml." if any(w in desc.lower() for w in ['perfil', 'tubo', 'guía', 'riel', 'varilla']) else "ud.",
                                            "p1": p, "p_t1": round(p * 0.75, 2), "img_path": ""
                                        })
                                except: pass
    except Exception as e:
        print(f"Aviso parseando Flexol: {e}")

    return items

def extract_all_catalog_precios(precios_dir="PRECIOS", output_json="catalog.json"):
    print(f"Extrayendo catálogo completo 2026 + Flexol + Mosquiflex desde: {precios_dir}")
    
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

    # 6. MOSQUIFLEX 2025 (PDF)
    mosq_pdf_files = [
        "MOSQUIFLEX® 2025.pdf",
        os.path.join(precios_dir, "MOSQUIFLEX® 2025.pdf"),
        os.path.join(precios_dir, "mosquiflex.pdf")
    ]
    mosq_pdf = next((f for f in mosq_pdf_files if os.path.exists(f)), None)
    if mosq_pdf:
        for it in extract_mosquiflex(mosq_pdf):
            c = it["code"]
            it["img_path"] = get_img(c, it["desc"], "Mosquiflex")
            products_map[c] = it

    # 7. FLEXOL 2025 (PDF)
    flex_pdf_files = [
        "FLEXOL 2025.pdf",
        os.path.join(precios_dir, "FLEXOL 2025.pdf"),
        os.path.join(precios_dir, "flexol.pdf")
    ]
    flex_pdf = next((f for f in flex_pdf_files if os.path.exists(f)), None)
    if flex_pdf:
        for it in extract_flexol(flex_pdf):
            c = it["code"]
            it["img_path"] = get_img(c, it["desc"], "Flexol")
            products_map[c] = it

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

    print(f"Catálogo completo generado con {len(all_products)} artículos en '{output_json}' y 'precios_catalogo.xlsx'.")
    return all_products

if __name__ == "__main__":
    p_dir = sys.argv[1] if len(sys.argv) > 1 else "PRECIOS"
    out_f = sys.argv[2] if len(sys.argv) > 2 else "catalog.json"
    extract_all_catalog_precios(p_dir, out_f)
