find_package(Qt6 REQUIRED COMPONENTS Quick)

# Camada que une as demais para o QML: serviços da aplicação, o controlador e o registro dos tipos.
qt_add_qml_module(bina_app
    URI Bina.App
    VERSION 1.0
    OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Bina/App
    STATIC
    SOURCES
        include/bina/app/AppController.h AppController.cpp
        include/bina/app/DocumentService.h DocumentService.cpp
        include/bina/app/ExportService.h ExportService.cpp
        include/bina/app/MessageLog.h MessageLog.cpp
        include/bina/app/Paths.h Paths.cpp
        include/bina/app/QmlTypes.h
)

add_library(Bina::app ALIAS bina_app)

target_include_directories(bina_app
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        # O registro de tipos gerado pelo Qt inclui os headers só pelo nome do arquivo.
        ${CMAKE_CURRENT_SOURCE_DIR}/include/bina/app
)

target_link_libraries(bina_app
    PUBLIC
        Qt6::Quick
        Bina::presentation
        Bina::model
        Bina::io
    PRIVATE
        bina_warnings
)
