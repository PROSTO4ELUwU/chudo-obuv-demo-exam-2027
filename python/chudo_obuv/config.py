"""Пути к ресурсам и параметры подключения к базе данных."""

import configparser
import sys
from dataclasses import dataclass
from pathlib import Path

CONFIG_FILE_NAME = "config.ini"


def _is_frozen() -> bool:
    """True, если приложение запущено из exe-файла, собранного PyInstaller."""
    return getattr(sys, "frozen", False)


def _application_dir() -> Path:
    """Папка с exe-файлом или с main.py."""
    if _is_frozen():
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parent.parent


def _resources_dir() -> Path:
    """Папка ресурсов: распакованная из exe-файла или в корне репозитория."""
    if _is_frozen():
        return Path(sys._MEIPASS) / "resources"
    return Path(__file__).resolve().parents[2] / "resources"


RESOURCES_DIR = _resources_dir()
IMAGES_DIR = RESOURCES_DIR / "images"
LOGO_FILE = RESOURCES_DIR / "logo.png"
ICON_FILE = RESOURCES_DIR / "icon.ico"
PLACEHOLDER_FILE = RESOURCES_DIR / "picture.png"


@dataclass(frozen=True)
class DatabaseSettings:
    """Параметры подключения к PostgreSQL."""

    host: str
    port: int
    dbname: str
    user: str
    password: str


def find_config_file() -> Path:
    """Ищет config.ini рядом с приложением и в двух родительских папках.

    Так один файл в корне репозитория подходит и для запуска
    из исходников (python/main.py), и для exe-файла из bin/python.
    """
    start = _application_dir()
    for folder in (start, *start.parents[:2]):
        candidate = folder / CONFIG_FILE_NAME
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(f"Файл настроек {CONFIG_FILE_NAME} не найден рядом с приложением")


def load_database_settings() -> DatabaseSettings:
    """Читает раздел [database] из config.ini."""
    parser = configparser.ConfigParser()
    parser.read(find_config_file(), encoding="utf-8")
    section = parser["database"]
    return DatabaseSettings(
        host=section.get("host", "localhost"),
        port=section.getint("port", 5432),
        dbname=section["dbname"],
        user=section["user"],
        password=section["password"],
    )
