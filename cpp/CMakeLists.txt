cmake_minimum_required(VERSION 3.21)

project(ChudoObuv
    VERSION 1.0.0
    DESCRIPTION "Система оформления заказа обуви «Чудо Обувь»"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Sql Test)
qt_standard_project_setup()

# Логика и окна собраны в библиотеку: её используют и приложение, и тесты
qt_add_library(chudo_obuv_core STATIC
    src/AppConfig.cpp src/AppConfig.h
    src/CatalogFilter.cpp src/CatalogFilter.h
    src/Database.cpp src/Database.h
    src/Errors.h
    src/Formatting.cpp src/Formatting.h
    src/Models.cpp src/Models.h
    src/Money.cpp src/Money.h
    src/OrderDraft.cpp src/OrderDraft.h
    src/Pricing.cpp src/Pricing.h
    src/Repositories.cpp src/Repositories.h
    src/ui/Application.cpp src/ui/Application.h
    src/ui/CatalogPage.cpp src/ui/CatalogPage.h
    src/ui/Images.cpp src/ui/Images.h
    src/ui/LoginPage.cpp src/ui/LoginPage.h
    src/ui/MainWindow.cpp src/ui/MainWindow.h
    src/ui/Messages.cpp src/ui/Messages.h
    src/ui/Page.cpp src/ui/Page.h
    src/ui/ProductCard.cpp src/ui/ProductCard.h
    src/ui/Style.cpp src/ui/Style.h
    src/ui/Widgets.cpp src/ui/Widgets.h
)
target_include_directories(chudo_obuv_core PUBLIC src)

# Ресурсы заказчика встраиваются в программу: логотип, иконка,
# картинка-заглушка и изображения товаров (в базе хранится имя файла)
set(RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../resources")
file(GLOB PRODUCT_IMAGES CONFIGURE_DEPENDS "${RESOURCES_DIR}/images/*.png")
qt_add_resources(chudo_obuv_core "customer_resources"
    PREFIX "/"
    BASE "${RESOURCES_DIR}"
    FILES
        "${RESOURCES_DIR}/logo.png"
        "${RESOURCES_DIR}/icon.ico"
        "${RESOURCES_DIR}/picture.png"
        ${PRODUCT_IMAGES})
target_link_libraries(chudo_obuv_core PUBLIC Qt6::Widgets Qt6::Sql)
target_compile_options(chudo_obuv_core PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>)

qt_add_executable(ChudoObuv WIN32 src/main.cpp app.rc)
target_link_libraries(ChudoObuv PRIVATE chudo_obuv_core)

enable_testing()
foreach(test_name tst_logic tst_repositories)
    qt_add_executable(${test_name} tests/${test_name}.cpp)
    set_target_properties(${test_name} PROPERTIES WIN32_EXECUTABLE OFF)
    target_link_libraries(${test_name} PRIVATE chudo_obuv_core Qt6::Test)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
