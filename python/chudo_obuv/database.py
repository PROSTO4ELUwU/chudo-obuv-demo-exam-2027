"""Подключение к базе данных PostgreSQL."""

import psycopg

from chudo_obuv.config import DatabaseSettings


class Database:
    """Единое подключение приложения к базе данных.

    Чтение выполняется в режиме автофиксации, а изменения — в явных
    транзакциях connection.transaction(), чтобы при ошибке ничего
    не сохранилось частично. Разорванное соединение (например, после
    перезапуска сервера) открывается заново при следующем обращении.
    """

    def __init__(self, settings: DatabaseSettings) -> None:
        self._settings = settings
        self._connection: psycopg.Connection | None = None

    @property
    def connection(self) -> psycopg.Connection:
        """Открытое соединение с базой данных."""
        if self._connection is None or self._connection.closed or self._connection.broken:
            self._connection = psycopg.connect(
                host=self._settings.host,
                port=self._settings.port,
                dbname=self._settings.dbname,
                user=self._settings.user,
                password=self._settings.password,
                autocommit=True,
                connect_timeout=5,
                application_name="chudo_obuv_python",
            )
        return self._connection

    def check_connection(self) -> None:
        """Проверяет, что сервер доступен и отвечает; иначе выбрасывает psycopg.Error."""
        self.connection.execute("SELECT 1")

    def close(self) -> None:
        """Закрывает соединение при выходе из приложения."""
        if self._connection is not None:
            self._connection.close()
