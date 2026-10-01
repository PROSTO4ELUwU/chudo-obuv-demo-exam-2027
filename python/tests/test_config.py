"""Чтение параметров подключения из config.ini."""

import pytest

from chudo_obuv.config import ConfigError, load_database_settings

REQUIRED = "dbname=chudo_obuv\nuser=app\npassword=secret\n"


def write_config(tmp_path, text):
    config_file = tmp_path / "config.ini"
    config_file.write_text(text, encoding="utf-8")
    return config_file


def test_host_and_port_have_defaults(tmp_path):
    settings = load_database_settings(write_config(tmp_path, "[database]\n" + REQUIRED))
    assert (settings.host, settings.port, settings.dbname) == ("localhost", 5432, "chudo_obuv")


def test_missing_parameter_is_named(tmp_path):
    config_file = write_config(tmp_path, "[database]\ndbname=chudo_obuv\nuser=app\n")
    with pytest.raises(ConfigError, match="не указан параметр password"):
        load_database_settings(config_file)


def test_port_must_be_integer(tmp_path):
    config_file = write_config(tmp_path, "[database]\nport=5432a\n" + REQUIRED)
    with pytest.raises(ConfigError, match="port должен быть целым числом"):
        load_database_settings(config_file)


def test_file_without_section_header(tmp_path):
    with pytest.raises(ConfigError, match="заполнен с ошибкой"):
        load_database_settings(write_config(tmp_path, REQUIRED))
