# Funções comuns das camadas de src/.

# Avisos do compilador, aplicados a todos os alvos do projeto (bibliotecas, app e testes).
add_library(bina_warnings INTERFACE)

if (MSVC)
    target_compile_options(bina_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(bina_warnings INTERFACE -Wall -Wextra)
endif()

# bina_add_library(<camada> SOURCES <arquivos...> DEPENDS <alvos...>)
#
# Cria a biblioteca estática Bina<Camada> com o alias Bina::<camada>. Os headers ficam em
# include/bina/<camada>/ e só esse diretório é público: uma camada enxerga apenas os headers das
# camadas que ela declara em DEPENDS.
function(bina_add_library layer)
    cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDS" ${ARGN})

    set(target bina_${layer})

    qt_add_library(${target} STATIC ${ARG_SOURCES})
    add_library(Bina::${layer} ALIAS ${target})

    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(${target} PUBLIC ${ARG_DEPENDS} PRIVATE bina_warnings)
endfunction()
