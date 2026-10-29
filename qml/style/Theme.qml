pragma Singleton

import QtQuick
import QtCore

// Constantes visuais do aplicativo: tokens de cor, tipografia, espaçamento, raios e animação. A
// aparência (sistema, claro ou escuro) e a cor de destaque são escolhidas pela pessoa e guardadas
// entre execuções; "sistema" acompanha o esquema claro/escuro do sistema.
QtObject {
    id: root

    readonly property string modeSystem: "system"
    readonly property string modeLight: "light"
    readonly property string modeDark: "dark"

    // Escolhas da pessoa (persistidas).
    property string mode: modeSystem
    property string accentId: "blue"

    // Cores de destaque disponíveis; cada uma tem o tom para o fundo claro e para o escuro.
    readonly property var accents: [
        { "id": "blue", "name": qsTr("Azul"), "light": "#1f6fd6", "dark": "#3b82e0" },
        { "id": "teal", "name": qsTr("Turquesa"), "light": "#0f7f8a", "dark": "#1f9aa6" },
        { "id": "green", "name": qsTr("Verde"), "light": "#1b7f4c", "dark": "#2a9d63" },
        { "id": "orange", "name": qsTr("Laranja"), "light": "#c25e0a", "dark": "#d9742a" },
        { "id": "purple", "name": qsTr("Roxo"), "light": "#7b3fb5", "dark": "#9a62d0" }
    ]

    readonly property bool systemDark: Qt.styleHints.colorScheme === Qt.ColorScheme.Dark
    readonly property bool dark: mode === modeDark || (mode === modeSystem && systemDark)

    // Fonte monoespaçada de cada plataforma; "monospace" é resolvida pelo fontconfig no Linux.
    readonly property string monoFont: Qt.platform.os === "windows" ? "Consolas"
                                     : Qt.platform.os === "osx" || Qt.platform.os === "ios" ? "Menlo"
                                     : "monospace"

    // Medidas.
    readonly property int rowHeight: 28
    readonly property int cellPadding: 8
    readonly property int pagePadding: 8
    readonly property int headerHeight: 34
    readonly property int controlHeight: 30
    readonly property real dimmedOpacity: 0.45

    // Espaçamentos.
    readonly property int spacingXs: 4
    readonly property int spacingSm: 6
    readonly property int spacingMd: 8
    readonly property int spacingLg: 12
    readonly property int spacingXl: 16

    // Raios dos cantos.
    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 6
    readonly property int radiusLarge: 10

    // Tipografia (o tamanho base é o da fonte do sistema).
    readonly property int fontSmall: 11
    readonly property int fontCaption: 12
    readonly property int fontTitle: 15

    // Animações.
    readonly property int animFast: 100
    readonly property int animNormal: 160

    // Formato que esconde o campo: ele é lido, mas não aparece no resultado (na estrutura fica esmaecido).
    readonly property string hiddenFormat: "hidden"

    // Superfícies, da mais funda para a mais elevada.
    readonly property color window: dark ? "#1e1f22" : "#f3f4f6"
    readonly property color surface: dark ? "#25272b" : "#ffffff"
    readonly property color surfaceAlt: dark ? "#2a2c31" : "#f6f7f9"
    readonly property color raised: dark ? "#2d3036" : "#ffffff"
    readonly property color control: dark ? "#33363c" : "#ffffff"
    readonly property color controlHover: dark ? "#3c4047" : "#eef1f5"
    readonly property color controlPressed: dark ? "#454a52" : "#e1e6ed"

    // Bordas.
    readonly property color border: dark ? "#3a3d43" : "#d9dde3"
    readonly property color borderStrong: dark ? "#555a63" : "#b4bbc5"

    // Texto.
    readonly property color text: dark ? "#e6e8eb" : "#1f2328"
    readonly property color textSecondary: dark ? "#9ba3ad" : "#5b6571"
    readonly property color textDisabled: dark ? "#6b717a" : "#9aa3ad"

    // Realces sobre qualquer superfície.
    readonly property color hoverOverlay: Qt.alpha(text, 0.07)
    readonly property color pressedOverlay: Qt.alpha(text, 0.14)
    readonly property color disabledFill: Qt.alpha(text, 0.1)
    readonly property color scrim: dark ? "#99000000" : "#59000000"
    readonly property color shadow: dark ? "#000000" : "#1a2230"
    readonly property real shadowOpacity: dark ? 0.55 : 0.28

    // Cor de destaque.
    readonly property color accent: {
        for (const item of accents) {
            if (item.id === accentId)
                return dark ? item.dark : item.light
        }
        return dark ? accents[0].dark : accents[0].light
    }
    readonly property color accentHover: dark ? Qt.lighter(accent, 1.12) : Qt.darker(accent, 1.12)
    readonly property color accentPressed: dark ? Qt.lighter(accent, 1.25) : Qt.darker(accent, 1.25)
    readonly property color accentText: "#ffffff"

    // Cabeçalhos de painel, destaque de ação pendente e selo de contagem.
    readonly property color headerBackground: dark ? "#2b2d31" : "#eceef2"
    readonly property color headerBorder: border
    readonly property color badgeBackground: dark ? "#3a3d42" : "#dde1e6"

    // Linha selecionada, passagem do mouse e contorno da seleção.
    // Opacos (misturados com a superfície): as células cobrem a lateral do contorno com o preenchimento.
    readonly property color selectionFill: Qt.tint(surface, Qt.alpha(accent, dark ? 0.28 : 0.13))
    readonly property color selectionBorder: Qt.tint(surface, Qt.alpha(accent, dark ? 0.8 : 0.55))
    readonly property color rowHover: Qt.tint(surface, Qt.alpha(text, dark ? 0.07 : 0.05))

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
    readonly property color errorBorder: dark ? "#8a3a3a" : "#e8a6a6"
    readonly property color errorText: dark ? "#f5b5b5" : "#7a1f1f"

    // Arrastar e soltar.
    readonly property color dropValid: dark ? "#5b9bf0" : "#2b7de9"
    readonly property color dropInvalid: dark ? "#e57373" : "#d64545"
    readonly property color ghostBackground: dark ? "#25324a" : "#e8f0fe"

    readonly property Settings store: Settings {
        category: "Appearance"

        property alias mode: root.mode
        property alias accentId: root.accentId
    }
}
