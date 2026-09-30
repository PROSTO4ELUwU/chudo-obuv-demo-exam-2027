SET client_encoding = 'UTF8';  -- psql в русской Windows иначе читает файл как WIN1251

-- ============================================================================
-- База данных «Чудо Обувь» — система оформления заказа обуви
-- СУБД: PostgreSQL 17
-- Структура в третьей нормальной форме с обеспечением ссылочной целостности.
-- Скрипт можно выполнять повторно: он пересоздаёт все таблицы.
-- ============================================================================

DROP TABLE IF EXISTS order_items, orders, stock_items, sizes, products,
    manufacturers, subcategories, categories, users, roles CASCADE;

-- ---------------------------------------------------------------------------
-- Справочники
-- ---------------------------------------------------------------------------

CREATE TABLE roles (
    role_id   integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    role_name varchar(50) NOT NULL UNIQUE
);
COMMENT ON TABLE roles IS 'Роли пользователей системы';

CREATE TABLE categories (
    category_id   integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    category_name varchar(100) NOT NULL UNIQUE
);
COMMENT ON TABLE categories IS 'Категории товаров (детская, женская, мужская обувь)';

CREATE TABLE subcategories (
    subcategory_id   integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    subcategory_name varchar(100) NOT NULL UNIQUE
);
COMMENT ON TABLE subcategories IS 'Подкатегории (виды) обуви: кроссовки, сапоги и т. д.; не зависят от категории';

CREATE TABLE manufacturers (
    manufacturer_id   integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    manufacturer_name varchar(100) NOT NULL UNIQUE
);
COMMENT ON TABLE manufacturers IS 'Производства (производители) обуви';

CREATE TABLE sizes (
    size_id    integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    size_value numeric(3, 1) NOT NULL UNIQUE CHECK (size_value > 0)
);
COMMENT ON TABLE sizes IS 'Размерная сетка обуви';
COMMENT ON COLUMN sizes.size_value IS 'Размер, в том числе половинчатый (36.5)';

-- ---------------------------------------------------------------------------
-- Пользователи
-- ---------------------------------------------------------------------------

CREATE TABLE users (
    user_id    integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    last_name  varchar(100) NOT NULL,
    first_name varchar(100) NOT NULL,
    patronymic varchar(100),
    login      varchar(50)  NOT NULL UNIQUE,
    role_id    integer      NOT NULL REFERENCES roles (role_id) ON DELETE RESTRICT
);
COMMENT ON TABLE users IS 'Пользователи системы; вход выполняется только по логину';
COMMENT ON COLUMN users.patronymic IS 'Отчество; может отсутствовать';

-- ---------------------------------------------------------------------------
-- Каталог
-- ---------------------------------------------------------------------------

CREATE TABLE products (
    product_id      integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    product_name    varchar(200)   NOT NULL,
    category_id     integer        NOT NULL REFERENCES categories (category_id) ON DELETE RESTRICT,
    subcategory_id  integer        NOT NULL REFERENCES subcategories (subcategory_id) ON DELETE RESTRICT,
    manufacturer_id integer        NOT NULL REFERENCES manufacturers (manufacturer_id) ON DELETE RESTRICT,
    description     text           NOT NULL,
    composition     text           NOT NULL,
    price           numeric(10, 2) NOT NULL CHECK (price > 0),
    image_file      varchar(255),
    CONSTRAINT uq_products_name_manufacturer UNIQUE (product_name, manufacturer_id)
);
COMMENT ON TABLE products IS 'Модели обуви в каталоге';
COMMENT ON COLUMN products.composition IS 'Состав (материалы верха, подкладки, подошвы)';
COMMENT ON COLUMN products.price IS 'Базовая цена в рублях без учёта скидки';
COMMENT ON COLUMN products.image_file IS 'Имя файла изображения в ресурсах; NULL — выводится картинка-заглушка';

CREATE TABLE stock_items (
    stock_item_id integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    product_id    integer NOT NULL REFERENCES products (product_id) ON DELETE CASCADE,
    size_id       integer NOT NULL REFERENCES sizes (size_id) ON DELETE RESTRICT,
    quantity      integer NOT NULL DEFAULT 0 CHECK (quantity >= 0),
    CONSTRAINT uq_stock_items_product_size UNIQUE (product_id, size_id)
);
COMMENT ON TABLE stock_items IS 'Товарные позиции — сочетания модели и размера';
COMMENT ON COLUMN stock_items.quantity IS 'Актуальное количество пар, доступное для заказа';

-- ---------------------------------------------------------------------------
-- Заказы
-- ---------------------------------------------------------------------------

CREATE TABLE orders (
    order_id   integer GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    order_date date    NOT NULL DEFAULT CURRENT_DATE,
    client_id  integer NOT NULL REFERENCES users (user_id) ON DELETE RESTRICT
);
COMMENT ON TABLE orders IS 'Заказы клиентов; итоговая сумма не хранится, а вычисляется по составу';
COMMENT ON COLUMN orders.order_id IS 'Номер заказа';

CREATE TABLE order_items (
    order_item_id integer        GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    order_id      integer        NOT NULL REFERENCES orders (order_id) ON DELETE CASCADE,
    stock_item_id integer        NOT NULL REFERENCES stock_items (stock_item_id) ON DELETE RESTRICT,
    quantity      integer        NOT NULL CHECK (quantity > 0),
    unit_price    numeric(10, 2) NOT NULL CHECK (unit_price > 0),
    CONSTRAINT uq_order_items_order_stock_item UNIQUE (order_id, stock_item_id)
);
COMMENT ON TABLE order_items IS 'Состав заказа';
COMMENT ON COLUMN order_items.unit_price IS 'Цена за единицу на момент заказа (с учётом действовавшей скидки)';

-- Скидка зависит от наличия заказов товара за прошлый месяц:
-- индексы ускоряют поиск заказов по дате и по товарной позиции.
CREATE INDEX ix_orders_order_date ON orders (order_date);
CREATE INDEX ix_order_items_stock_item_id ON order_items (stock_item_id);
