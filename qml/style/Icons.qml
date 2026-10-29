pragma Singleton

import QtQuick

// Ícones do aplicativo: glifos da fonte Material Design Icons (fonts/materialdesignicons-webfont.ttf).
// Só entram aqui os glifos em uso; os códigos vêm de fonts/materialdesignicons-webfont.qml, gerado pelo
// fontdrop-glyph (https://github.com/rafarisson/fontdrop-glyph) com todos os glifos da fonte. Para
// usar outro ícone, copie a linha dele de lá. Os glifos ficam na área de uso privado do Unicode, que
// muitos editores não desenham: por isso são escritos como pares substitutos (\uD...\uD...).
QtObject {
    readonly property FontLoader loader: FontLoader {
        source: "qrc:/fonts/materialdesignicons-webfont.ttf"
    }
    readonly property string family: loader.name

    readonly property string menu: "\uDB80\uDF5C"
    readonly property string contentSave: "\uDB80\uDD93"
    readonly property string folderOpenOutline: "\uDB83\uDDCF"
    readonly property string play: "\uDB81\uDC0A"
    readonly property string fileExportOutline: "\uDB84\uDC2E"
    readonly property string cog: "\uDB81\uDC93"
    readonly property string bellOutline: "\uDB80\uDC9C"
    readonly property string expandAll: "\uDB82\uDEB5"
    readonly property string collapseAll: "\uDB82\uDEA7"
    readonly property string restore: "\uDB82\uDD9B"
    readonly property string alert: "\uDB80\uDC26"
    readonly property string close: "\uDB80\uDD56"
    readonly property string chevronDown: "\uDB80\uDD40"
    readonly property string chevronRight: "\uDB80\uDD42"
    readonly property string contentCopy: "\uDB80\uDD8F"
    readonly property string fileTree: "\uDB81\uDE45"
    readonly property string tableLarge: "\uDB81\uDCF1"
    readonly property string check: "\uDB80\uDD2C"
    readonly property string fileTreeOutline: "\uDB84\uDFD2"
    readonly property string fileSearchOutline: "\uDB83\uDC7D"
    readonly property string monitor: "\uDB80\uDF79"
    readonly property string whiteBalanceSunny: "\uDB81\uDDA8"
    readonly property string weatherNight: "\uDB81\uDD94"
}
