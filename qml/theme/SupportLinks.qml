pragma Singleton
import QtQuick

QtObject {
    // SUPPORT_LINKS_SINGLE_SOURCE_V1 — keep every public SonKuPik destination
    // consistent across About and voluntary-support surfaces.
    readonly property string youtubeUrl: "https://www.youtube.com/@sonkupik"
    readonly property string tokopediaUrl: "https://www.tokopedia.com/dr-sonkupik/recording-tech-ktv-pro-k500-karaoke-effect-processor-4-input-6-output-digital-mixer-dengan-equalizer-compressor-anti-feedback-crossover-ktv-pro-k500"

    // Identity shown beside the official static QRIS. The payment image itself
    // is never generated from these labels; it must be supplied as a verified
    // merchant-issued PNG and bundled by CMake.
    readonly property string merchantName: "SONKUPIK, AUDIO DEVELOPER, DIGITAL & KREATIF"
    readonly property string merchantNmid: "ID1026551401775"
    readonly property url qrisSource: "qrc:/support/qris-sonkupik.png"

    // QRIS_CROP_20260718_V1 — crop the owner-supplied 1240x1748 official
    // merchant poster to its QR region for reliable phone scanning at a compact
    // dialog size. Merchant identity remains rendered separately below.
    readonly property rect qrisCrop: Qt.rect(138, 438, 964, 964)
}
