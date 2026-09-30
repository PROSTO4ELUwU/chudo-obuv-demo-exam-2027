# Сборка exe-файла Python-версии: python -m PyInstaller --noconfirm ChudoObuv.spec
# (удобнее через build_exe.bat). Spec-файл нужен, чтобы не включать в сборку
# части Qt, которые приложение не использует: без них exe вдвое меньше.
from pathlib import Path

PYTHON_DIR = Path(SPECPATH)
ROOT = PYTHON_DIR.parent

# QML, PDF, сеть, OpenGL, SVG и экранная клавиатура приложению не нужны
UNUSED_QT_FILES = ("opengl32sw", "Qt6Quick", "Qt6Qml", "Qt6Pdf", "Qt6OpenGL", "Qt6Network",
                   "Qt6Svg", "Qt6VirtualKeyboard", "QtNetwork")
# OpenSSL для сетевых плагинов Qt: PyInstaller находит его через PATH
# (например, из Git). OpenSSL самого Python (libssl-3.dll) и psycopg остаются
UNUSED_TOP_LEVEL_FILES = ("libssl-3-x64.dll", "libcrypto-3-x64.dll")
# Плагины, которые зависят от исключённых модулей
UNUSED_PLUGIN_TYPES = ("tls", "networkinformation", "iconengines", "platforminputcontexts",
                       "generic")


def is_needed(entry: tuple) -> bool:
    """Оставляет в сборке только то, что использует приложение."""
    parts = entry[0].replace("\\", "/").split("/")
    file_name = parts[-1]
    if file_name.startswith(UNUSED_QT_FILES) or entry[0] in UNUSED_TOP_LEVEL_FILES:
        return False
    if "plugins" in parts:
        plugin_type = parts[parts.index("plugins") + 1]
        if plugin_type in UNUSED_PLUGIN_TYPES:
            return False
        # PNG Qt читает сам, плагин нужен только для иконки .ico
        if plugin_type == "imageformats":
            return file_name.startswith("qico")
    if "translations" in parts:
        # Нужен только русский перевод стандартных кнопок и диалогов
        return file_name.startswith("qtbase_ru")
    return True


a = Analysis(
    [str(PYTHON_DIR / "main.py")],
    pathex=[str(PYTHON_DIR)],
    datas=[(str(ROOT / "resources"), "resources")],
    excludes=["tkinter", "unittest"],
)
a.binaries = [entry for entry in a.binaries if is_needed(entry)]
a.datas = [entry for entry in a.datas if is_needed(entry)]

pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    name="ChudoObuv",
    icon=str(ROOT / "resources" / "icon.ico"),
    console=False,
    upx=False,
)
