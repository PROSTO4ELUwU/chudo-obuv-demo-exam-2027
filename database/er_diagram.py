"""ER-диаграмма базы данных «Чудо Обувь» по метаданным PostgreSQL.

Скрипт читает из системного каталога СУБД таблицы, столбцы, первичные
и внешние ключи, описывает диаграмму на языке Graphviz (er_diagram.dot)
и строит из неё er_diagram.pdf (по заданию) и er_diagram.png (для README).

Запуск из корня репозитория (нужен Graphviz: программа dot в PATH):
    python database/er_diagram.py
"""

import configparser
import shutil
import subprocess
from collections import defaultdict
from html import escape
from pathlib import Path

import psycopg

DATABASE_DIR = Path(__file__).resolve().parent
CONFIG_FILE = DATABASE_DIR.parent / "config.ini"
DOT_FILE = DATABASE_DIR / "er_diagram.dot"
PDF_FILE = DATABASE_DIR / "er_diagram.pdf"
PNG_FILE = DATABASE_DIR / "er_diagram.png"

ACCENT_COLOR = "#70B2AF"
EXTRA_BACKGROUND = "#D2F6E7"

COLUMNS_QUERY = """
    SELECT rel.relname, att.attname, format_type(att.atttypid, att.atttypmod)
    FROM pg_attribute AS att
    JOIN pg_class AS rel ON rel.oid = att.attrelid
    JOIN pg_namespace AS nsp ON nsp.oid = rel.relnamespace
    WHERE nsp.nspname = 'public' AND rel.relkind = 'r'
      AND att.attnum > 0 AND NOT att.attisdropped
    ORDER BY rel.relname, att.attnum
"""

# Для первичного ключа confkey пуст, и unnest дополняет его значениями NULL
KEYS_QUERY = """
    SELECT con.contype, rel.relname, att.attname, ref.relname, ref_att.attname
    FROM pg_constraint AS con
    JOIN pg_class AS rel ON rel.oid = con.conrelid
    JOIN pg_namespace AS nsp ON nsp.oid = rel.relnamespace
    CROSS JOIN LATERAL unnest(con.conkey, con.confkey) AS k (attnum, ref_attnum)
    JOIN pg_attribute AS att ON att.attrelid = con.conrelid AND att.attnum = k.attnum
    LEFT JOIN pg_class AS ref ON ref.oid = con.confrelid
    LEFT JOIN pg_attribute AS ref_att
        ON ref_att.attrelid = con.confrelid AND ref_att.attnum = k.ref_attnum
    WHERE nsp.nspname = 'public' AND con.contype IN ('p', 'f')
"""


def connect() -> psycopg.Connection:
    """Подключается к базе по параметрам из config.ini."""
    config = configparser.ConfigParser()
    config.read(CONFIG_FILE, encoding="utf-8")
    return psycopg.connect(**config["database"])


def table_node(table: str, columns: list[tuple[str, str]],
               primary_keys: set[tuple[str, str]], foreign_keys: set[tuple[str, str]]) -> str:
    """Описание таблицы в виде HTML-метки Graphviz.

    У каждой строки два порта: входящая связь подходит к левой ячейке,
    исходящая выходит из правой, поэтому линии не пересекают строку.
    """
    rows = [
        f'<TR><TD COLSPAN="3" BGCOLOR="{ACCENT_COLOR}">'
        f'<FONT COLOR="white"><B>{table}</B></FONT></TD></TR>'
    ]
    for column, data_type in columns:
        is_primary = (table, column) in primary_keys
        marks = ", ".join(mark for mark, present in (
            ("PK", is_primary),
            ("FK", (table, column) in foreign_keys),
        ) if present)
        background = f' BGCOLOR="{EXTRA_BACKGROUND}"' if is_primary else ""
        rows.append(
            f'<TR><TD ALIGN="LEFT" PORT="{column}_in"{background}>{marks}</TD>'
            f'<TD ALIGN="LEFT"{background}>{column}</TD>'
            f'<TD ALIGN="LEFT" PORT="{column}_out"{background}>'
            f'<FONT COLOR="#555555">{escape(data_type)}</FONT></TD></TR>'
        )
    table_html = "".join(rows)
    return (f'    {table} [label=<<TABLE BORDER="0" CELLBORDER="1" CELLSPACING="0" '
            f'CELLPADDING="4">{table_html}</TABLE>>];')


def build_dot(connection: psycopg.Connection) -> str:
    """Формирует текст диаграммы по метаданным базы."""
    columns: dict[str, list[tuple[str, str]]] = defaultdict(list)
    for table, column, data_type in connection.execute(COLUMNS_QUERY):
        columns[table].append((column, data_type))

    primary_keys: set[tuple[str, str]] = set()
    references: list[tuple[str, str, str, str]] = []
    for kind, table, column, ref_table, ref_column in connection.execute(KEYS_QUERY):
        if kind == "p":
            primary_keys.add((table, column))
        else:
            references.append((table, column, ref_table, ref_column))
    foreign_keys = {(table, column) for table, column, _, _ in references}

    nodes = [table_node(table, table_columns, primary_keys, foreign_keys)
             for table, table_columns in columns.items()]
    # Нотация «воронья лапка»: у ссылающейся таблицы «ноль или много»,
    # у таблицы-справочника «ровно один» (все внешние ключи NOT NULL)
    edges = [f"    {table}:{column}_out:e -> {ref_table}:{ref_column}_in:w;"
             for table, column, ref_table, ref_column in sorted(references)]
    return "\n".join([
        "digraph er_diagram {",
        '    graph [rankdir=LR, nodesep=0.35, ranksep=1.1, pad=0.3, fontname="Calibri",',
        '           labelloc=t, fontsize=20, label=<<B>ER-диаграмма базы данных «Чудо Обувь»</B>'
        '<BR/><FONT POINT-SIZE="12">PK — первичный ключ, FK — внешний ключ; '
        'связи «один ко многим» в нотации «воронья лапка»</FONT>>];',
        '    node [shape=plain, fontname="Calibri", fontsize=11];',
        '    edge [dir=both, arrowtail=crowodot, arrowhead=teetee, color="#444444"];',
        *nodes,
        *edges,
        "}",
        "",
    ])


def main() -> None:
    """Строит er_diagram.dot, er_diagram.pdf и er_diagram.png."""
    dot = shutil.which("dot")
    if dot is None:
        raise SystemExit("Не найден Graphviz: установите его и добавьте каталог bin в PATH")
    with connect() as connection:
        DOT_FILE.write_text(build_dot(connection), encoding="utf-8", newline="\n")
    subprocess.run([dot, "-Tpdf", str(DOT_FILE), "-o", str(PDF_FILE)], check=True)
    subprocess.run([dot, "-Tpng", "-Gdpi=110", str(DOT_FILE), "-o", str(PNG_FILE)], check=True)
    print(f"Диаграмма построена: {PDF_FILE}, {PNG_FILE.name}")


if __name__ == "__main__":
    main()
