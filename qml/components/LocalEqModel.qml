import QtQuick

ListModel {
    id: root

    property int bandCount: 5
    property real hpfHz: defaultHpfHz
    property real lpfHz: defaultLpfHz
    property string hpType: defaultHpType
    property string lpType: defaultLpType
    property real defaultHpfHz: 20
    property real defaultLpfHz: 20000
    property string defaultHpType: "HP Butter 12"
    property string defaultLpType: "LP Butter 12"
    property var defaultFrequencies: []

    signal bandChanged()
    signal crossoverChanged()

    function defaultsForCount() {
        if (defaultFrequencies && defaultFrequencies.length === bandCount)
            return defaultFrequencies
        if (bandCount === 10) return [80, 125, 250, 500, 1000, 2000, 4000, 6300, 10000, 12500]
        if (bandCount === 7) return [80, 160, 315, 630, 1250, 2500, 8000]
        return [125, 250, 1000, 2500, 8000]
    }

    function rebuild() {
        clear()
        var freqs = defaultsForCount()
        for (var i = 0; i < bandCount; ++i) {
            append({ freq: freqs[i], gain: 0.0, q: 1.0, typeName: "BELL" })
        }
        bandChanged()
    }

    function setBand(index, frequency, gain, q) {
        if (index < 0 || index >= count) return
        setProperty(index, "freq", Math.max(20, Math.min(20000, Number(frequency))))
        setProperty(index, "gain", Math.max(-24, Math.min(24, Number(gain))))
        setProperty(index, "q", Math.max(0.1, Math.min(30, Number(q))))
        bandChanged()
    }

    function setBandType(index, typeName) {
        if (index < 0 || index >= count) return
        setProperty(index, "typeName", typeName)
        bandChanged()
    }

    function resetBand(index) {
        if (index < 0 || index >= count) return
        var freqs = defaultsForCount()
        set(index, { freq: freqs[index], gain: 0.0, q: 1.0, typeName: "BELL" })
        bandChanged()
    }

    function resetAll() {
        rebuild()
        hpfHz = defaultHpfHz
        lpfHz = defaultLpfHz
        hpType = defaultHpType
        lpType = defaultLpType
        crossoverChanged()
    }

    function setHpfHz(value) {
        hpfHz = Math.max(20, Math.min(20000, Number(value)))
        crossoverChanged()
    }

    function setLpfHz(value) {
        lpfHz = Math.max(20, Math.min(20000, Number(value)))
        crossoverChanged()
    }

    function setHpType(value) { hpType = String(value); crossoverChanged() }
    function setLpType(value) { lpType = String(value); crossoverChanged() }

    onBandCountChanged: rebuild()
    Component.onCompleted: rebuild()
}
