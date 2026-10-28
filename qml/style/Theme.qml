pragma Singleton

import QtQuick

// Constantes visuais do aplicativo. As cores acompanham o esquema claro/escuro do sistema.
QtObject {
    readonly property bool dark: Qt.styleHints.colorScheme === Qt.ColorScheme.Dark

    // Fonte monoespaçada de cada plataforma; "monospace" é resolvida pelo fontconfig no Linux.
    readonly property string monoFont: Qt.platform.os === "windows" ? "Consolas"
                                     : Qt.platform.os === "osx" || Qt.platform.os === "ios" ? "Menlo"
                                     : "monospace"

    readonly property int rowHeight: 28
    readonly property int cellPadding: 8
    readonly property int pagePadding: 8
    readonly property int headerHeight: 34
    readonly property real dimmedOpacity: 0.45

    // Formato que esconde o campo: ele é lido, mas não aparece no resultado (na estrutura fica esmaecido).
    readonly property string hiddenFormat: "hidden"

    // Cabeçalhos de painel, destaque de ação pendente e selo de contagem.
    readonly property color headerBackground: dark ? "#2b2d31" : "#f1f3f5"
    readonly property color headerBorder: dark ? "#3a3d42" : "#d5d9de"
    readonly property color accent: dark ? "#3b82e0" : "#1f6fd6"
    readonly property color accentText: "#ffffff"
    readonly property color badgeBackground: dark ? "#3a3d42" : "#dde1e6"

    // Cor da coluna de tipo, por categoria.
    readonly property color typeInteger: dark ? "#6cb6ff" : "#1a5fb4"
    readonly property color typeFloat: dark ? "#d2a8ff" : "#7b3fb5"
    readonly property color typeText: dark ? "#7ee2a8" : "#1b7f4c"
    readonly property color typeStruct: dark ? "#f0b072" : "#a85a0a"
    readonly property color typeCustom: dark ? "#e6a0c4" : "#a3306b"

    function typeColor(name, fallback) {
        if (name === "char" || name === "string")
            return typeText
        if (name === "float" || name === "double")
            return typeFloat
        if (/^u?int\d+$/.test(name) || name === "bool")
            return typeInteger
        if (name === "struct" || name === "registro")
            return typeStruct
        return name === "" ? fallback : typeCustom
    }

    // Larguras iniciais das colunas, na ordem das colunas dos models.
    readonly property list<int> schemaColumnWidths: [190, 110, 70, 70, 100]
    readonly property list<int> resultColumnWidths: [200, 110, 80, 60, 110, 380]
    readonly property int defaultColumnWidth: 80

    // Aviso de dados desatualizados e mensagens de erro.
    readonly property color warningBackground: dark ? "#3d3415" : "#fff3cd"
    readonly property color warningBorder: dark ? "#8a7426" : "#e0c36a"
    readonly property color warningText: dark ? "#f2dc8b" : "#664d03"
    readonly property color errorBackground: dark ? "#4a1f1f" : "#fde8e8"
    readonly property color errorText: dark ? "#f5b5b5" : "#7a1f1f"

    // Arrastar e soltar.
    readonly property color dropValid: dark ? "#5b9bf0" : "#2b7de9"
    readonly property color dropInvalid: dark ? "#e57373" : "#d64545"
    readonly property color ghostBackground: dark ? "#25324a" : "#e8f0fe"
}
