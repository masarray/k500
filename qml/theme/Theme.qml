pragma Singleton
import QtQuick

QtObject {
    // TYPOGRAPHY_SINGLE_FAMILY_V1
    // Every UI role intentionally resolves to the same embedded application font.
    readonly property string fontFamily: "Plus Jakarta Sans"
    readonly property string displayFamily: "Plus Jakarta Sans"
    readonly property string monoFamily: "Plus Jakarta Sans"

    // Exact sRGB equivalents of the current web console OKLCH tokens.
    readonly property color bg: "#060A0E"
    readonly property color background: bg
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

    // VISUAL_FOUNDATION_V1: semantic, compact-density roles for new refinement.
    // Existing sizes remain unchanged until each component is audited.
    readonly property int navTitleSize: 12
    readonly property int navSubtitleSize: 11

    // RACK_TYPE_HIERARCHY_V1 — optical roles without larger controls or racks.
    readonly property int rackHeaderSize: 10
    readonly property real rackHeaderTracking: 0.75
    readonly property int rackCaptionSize: 9
    readonly property int rackReadoutSize: 10
    readonly property int rackUnitSize: 8
    readonly property color rackReadoutColor: amber

    // SYSTEM_DASHBOARD_OPTICAL_V2B — text hierarchy, never device authority.
    readonly property int systemListTitleSize: 10
    readonly property int systemCaptionSize: 9
    readonly property int systemStatusSize: 9
    readonly property color systemSelectedSurface: "#13272C"
    readonly property color systemActiveSurface: "#242017"
    readonly property int navHeaderSize: 10
    readonly property int navItemHeight: 48
    readonly property int navIconSize: 27
    readonly property color navActiveSurface: "#17262B"
    readonly property color navHoverSurface: "#182127"
    readonly property color navIdleSurface: "transparent"
    readonly property color navAccentRail: accent
    readonly property color navActiveText: accent
    readonly property color navSubtitleText: "#A4AEB7"
}
