SET client_encoding = 'UTF8';  -- psql в русской Windows иначе читает файл как WIN1251

-- ============================================================================
-- Роль, от имени которой приложения подключаются к базе chudo_obuv.
-- Выполняется суперпользователем после chudo_obuv.sql (пересоздание таблиц
-- удаляет выданные на них права, поэтому скрипт запускается каждый раз).
-- ============================================================================

DO $$
BEGIN
    IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'chudo_obuv_app') THEN
        CREATE ROLE chudo_obuv_app LOGIN;
    END IF;
END $$;

-- Пароль совпадает с указанным в config.ini
ALTER ROLE chudo_obuv_app PASSWORD 'Izk7MAn1ZKY8mEjxjMTa';

GRANT CONNECT ON DATABASE chudo_obuv TO chudo_obuv_app;
GRANT USAGE ON SCHEMA public TO chudo_obuv_app;
GRANT SELECT ON ALL TABLES IN SCHEMA public TO chudo_obuv_app;

-- Изменять приложение может только заказы и остатки товарных позиций
GRANT INSERT, UPDATE, DELETE ON orders, order_items TO chudo_obuv_app;
GRANT UPDATE (quantity) ON stock_items TO chudo_obuv_app;
