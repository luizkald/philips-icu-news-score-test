include(FetchContent)

# GoogleTest via FetchContent (sem instalação manual).
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

# Evita sobrescrever as configurações de runtime do compilador no Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Não inclui main.cpp; reaproveita as fontes de produção necessárias.
add_executable(unit_tests
    test_news.cpp
    test_icu.cpp
    ../src/News.cpp
    ../src/ICU.cpp
)

target_include_directories(unit_tests PRIVATE ../include)
target_link_libraries(unit_tests PRIVATE GTest::gtest_main)

# Testes end-to-end: executam o binário real (icu_news) alimentando a stdin com
# as demos e cenários que cobrem todos os caminhos da CLI.
add_executable(e2e_tests
    test_e2e.cpp
)

target_link_libraries(e2e_tests PRIVATE GTest::gmock GTest::gtest_main)

# Caminho do binário e diretório das demos, resolvidos pelo CMake.
target_compile_definitions(e2e_tests PRIVATE
    ICU_NEWS_BINARY="$<TARGET_FILE:icu_news>"
    DEMO_DIR="${CMAKE_SOURCE_DIR}"
)

# Garante que o app esteja compilado antes de rodar os testes E2E.
add_dependencies(e2e_tests icu_news)

include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(e2e_tests)
