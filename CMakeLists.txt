cmake_minimum_required(VERSION 3.16)

project(BinAnalyzer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BINA_BUILD_TESTS "Compila e registra os testes (tests/)" OFF)
option(BINA_UI_TEST_HOOKS "Habilita BINA_SCREENSHOT=arquivo.png para capturar a janela em testes de interface" OFF)

find_package(Qt6 REQUIRED COMPONENTS Quick LinguistTools)

# Os textos para o usuário são escritos em português (idioma-fonte) em todas as camadas; as outras
# línguas vêm dos arquivos de i18n/ (ver qt_add_translations no fim deste arquivo).
qt_standard_project_setup(REQUIRES 6.10
    I18N_SOURCE_LANGUAGE pt_BR
    I18N_TRANSLATED_LANGUAGES en
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(BinaHelpers)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

# Bibliotecas das camadas (ver src/CMakeLists.txt).
add_subdirectory(src)

qt_add_executable(BinAnalyzerApp
    app/main.cpp
)

set(BINA_QML_FILES
    qml/Main.qml
    qml/AppToolBar.qml
    qml/AppStatusBar.qml
    qml/style/Theme.qml
    qml/style/Icons.qml
    qml/controls/Icon.qml
    qml/controls/IconButton.qml
    qml/controls/RowBackground.qml
    qml/controls/SelectableTreeView.qml
    qml/panes/SchemaPane.qml
    qml/panes/TypePane.qml
    qml/panes/FormatPane.qml
    qml/panes/PaneHeader.qml
    qml/panes/TreeActions.qml
    qml/panes/ResultPane.qml
    qml/panes/ViewSwitcher.qml
    qml/tree/SchemaTreeView.qml
    qml/tree/SchemaDelegate.qml
    qml/tree/ResultTreeView.qml
    qml/tree/ResultTreeDelegate.qml
    qml/tree/ResultTableView.qml
    qml/tree/ResultDetails.qml
    qml/tree/EmptyState.qml
    qml/tree/TreeContextMenu.qml
    qml/tree/TreeDragAdapter.qml
    qml/tree/DropIndicator.qml
    qml/tree/DragGhost.qml
    qml/editors/CellEditDelegate.qml
    qml/editors/TextCellEditor.qml
    qml/editors/ComboCellEditor.qml
    qml/editors/SpinCellEditor.qml
    qml/popups/OptionsPopup.qml
    qml/popups/MessagesPopup.qml
    qml/popups/NewItemPopup.qml
    qml/popups/ConfirmPopup.qml
    qml/popups/StatusToast.qml
    qml/popups/StaleBanner.qml
)

# Os arquivos ficam em subpastas por assunto, mas no recurso todos ficam no mesmo diretório: assim
# continuam se enxergando pelo nome, sem imports entre as pastas.
foreach(file IN LISTS BINA_QML_FILES)
    get_filename_component(name ${file} NAME)
    set_source_files_properties(${file} PROPERTIES QT_RESOURCE_ALIAS ${name})
endforeach()

set_source_files_properties(qml/style/Theme.qml qml/style/Icons.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(BinAnalyzerApp
    URI BinAnalyzer
    VERSION 1.0
    QML_FILES
        ${BINA_QML_FILES}
    DEPENDENCIES
        Bina.App
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(BinAnalyzerApp PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.BinAnalyzerApp
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(BinAnalyzerApp
    PRIVATE
        Qt6::Quick
        Bina::app
        bina_warnings
)

# A fonte dos ícones (ver qml/style/Icons.qml). O fonts/materialdesignicons-webfont.qml, gerado pelo
# fontdrop-glyph, só serve de consulta aos códigos dos glifos e não entra no programa.
qt_add_resources(BinAnalyzerApp "fonts"
    PREFIX "/fonts"
    BASE fonts
    FILES
        fonts/materialdesignicons-webfont.ttf
)

# Os exemplos abertos quando o programa é iniciado sem argumentos vão embutidos no executável: assim
# funcionam em qualquer máquina, não só onde o projeto foi compilado.
qt_add_resources(BinAnalyzerApp "samples"
    PREFIX "/samples"
    BASE samples
    FILES
        samples/elements.json
        samples/elements.bin
)

# Os tipos padrão da biblioteca de tipos: copiados para a pasta do usuário na primeira execução.
qt_add_resources(BinAnalyzerApp "types"
    PREFIX "/types"
    BASE samples
    FILES
        samples/struct_tm.json
)

if (BINA_UI_TEST_HOOKS)
    target_compile_definitions(BinAnalyzerApp PRIVATE BINA_UI_TEST_HOOKS)
endif()

# Traduções. "cmake --build . --target update_translations" atualiza os .ts com os textos de todas as
# camadas; os .qm vão embutidos no executável em :/i18n.
qt_add_translations(BinAnalyzerApp
    SOURCE_TARGETS
        BinAnalyzerApp
        bina_schema bina_format bina_parse bina_document bina_session bina_io bina_model
        bina_presentation bina_app
    TS_FILE_BASE BinAnalyzer
    TS_FILE_DIR i18n
    RESOURCE_PREFIX /i18n
)

install(TARGETS BinAnalyzerApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (BINA_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()
