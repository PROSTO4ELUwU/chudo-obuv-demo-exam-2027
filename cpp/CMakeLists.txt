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
    src/Database.cpp src/Database.h
    src/Errors.h
    src/Formatting.cpp src/Formatting.h
    src/Money.cpp src/Money.h
    src/Pricing.cpp src/Pricing.h
)
target_include_directories(chudo_obuv_core PUBLIC src)
target_link_libraries(chudo_obuv_core PUBLIC Qt6::Widgets Qt6::Sql)
target_compile_options(chudo_obuv_core PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>)

qt_add_executable(ChudoObuv WIN32 src/main.cpp)
target_link_libraries(ChudoObuv PRIVATE chudo_obuv_core)

enable_testing()
qt_add_executable(tst_logic tests/tst_logic.cpp)
set_target_properties(tst_logic PROPERTIES WIN32_EXECUTABLE OFF)
target_link_libraries(tst_logic PRIVATE chudo_obuv_core Qt6::Test)
add_test(NAME tst_logic COMMAND tst_logic)
