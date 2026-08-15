pragma Singleton
import QtQuick

QtObject {
    readonly property string fontFamily: "Segoe UI Variable Text"
    readonly property string displayFamily: "Segoe UI Variable Display"
    readonly property string monoFamily: "Cascadia Mono"

    // Exact sRGB equivalents of the current web console OKLCH tokens.
    readonly property color bg: "#060A0E"
    readonly property color chassis: "#0C1116"
    readonly property color bgRaised: "#11171D"
    readonly property color panel: "#11171D"
    readonly property color panelRaised: "#151B22"
    readonly property color recessed: "#040507"
    readonly property color control: "#1A2026"
    readonly property color controlRaised: "#2D343A"
    readonly property color controlHover: "#212A33"
    readonly property color border: "#2B343D"
    readonly property color borderSoft: "#1D252C"
    readonly property color highlight: "#46525C"
    readonly property color focus: "#24E9F2"

    readonly property color text: "#E3E8EE"
    readonly property color textSoft: "#C9D1D8"
    readonly property color textDim: "#8A939D"
    readonly property color textFaint: "#5B6873"

    readonly property color accent: "#24E9F2"
    readonly property color accentSoft: "#7324E9F2"
    readonly property color accentFaint: "#1F24E9F2"
    readonly property color amber: "#FFB200"
    readonly property color amberSoft: "#66FFB200"
    readonly property color amberFaint: "#1FFFB200"
    readonly property color blue: "#69AEEA"
    readonly property color violet: "#A58AE8"
    readonly property color red: "#F36B6B"
    readonly property color green: "#57D49A"

    readonly property int radiusSmall: 5
    readonly property int radius: 8
    readonly property int radiusLarge: 10

    readonly property int gapXS: 4
    readonly property int gapS: 7
    readonly property int gap: 10
    readonly property int gapL: 14

    readonly property int textXS: 9
    readonly property int textS: 10
    readonly property int textM: 11
    readonly property int textL: 13
    readonly property int textXL: 16
}
