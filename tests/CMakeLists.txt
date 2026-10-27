find_package(Qt6 REQUIRED COMPONENTS Test)

# bina_add_test(<nome> <camada> <fontes...>)
#
# Um executável por assunto; cada um é um teste do CTest (ctest --output-on-failure). Cada teste
# linka só a camada que testa (e o que ela traz), o que também confirma as fronteiras entre camadas.
function(bina_add_test name layer)
    qt_add_executable(${name} ${ARGN} TestSupport.h)

    target_link_libraries(${name}
        PRIVATE
            Qt6::Test
            ${layer}
            bina_warnings
    )

    target_compile_definitions(${name} PRIVATE BINA_SAMPLES_DIR="${PROJECT_SOURCE_DIR}/samples")

    set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE FALSE)

    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

# Testes que montam documentos a partir de JSON (TestSupport::documentFromJson) usam Bina::io.
bina_add_test(tst_format          Bina::format    tst_format.cpp)
bina_add_test(tst_layout          Bina::io        tst_layout.cpp)
bina_add_test(tst_parser          "Bina::parse;Bina::io"   tst_parser.cpp)
bina_add_test(tst_document        Bina::io        tst_document.cpp)
bina_add_test(tst_schematreemodel "Bina::model;Bina::io"   tst_schematreemodel.cpp)
bina_add_test(tst_session         "Bina::model;Bina::io"   tst_session.cpp)
bina_add_test(tst_appcontroller   Bina::app       tst_appcontroller.cpp)

# Os exemplos também como recurso, como no executável (abrir ":/samples/..." como modelo).
qt_add_resources(tst_appcontroller "samples"
    PREFIX "/samples"
    BASE ${PROJECT_SOURCE_DIR}/samples
    FILES
        ${PROJECT_SOURCE_DIR}/samples/elements.json
        ${PROJECT_SOURCE_DIR}/samples/elements.bin
)
