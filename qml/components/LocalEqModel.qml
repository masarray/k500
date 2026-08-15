import QtQuick

ListModel {
    id: root

    property int bandCount: 5
    property real hpfHz: 20
    property real lpfHz: 20000
    property string hpType: "HP LR 24"
    property string lpType: "LP LR 24"
    property var defaultFrequencies: []

    signal bandChanged()
    signal crossoverChanged()

    function defaultsForCount() {
        if (defaultFrequencies && defaultFrequencies.length === bandCount)
            return defaultFrequencies
        if (bandCount === 10) return [80, 125, 250, 500, 1000, 2000, 4000, 6300, 10000, 12500]
        if (bandCount === 7) return [80, 160, 315, 630, 1250, 2500, 8000]
        return [80, 250, 800, 2500, 8000]
    }

    function rebuild() {
        clear()
        var freqs = defaultsForCount()
        for (var i = 0; i < bandCount; ++i) {
            append({
                freq: freqs[i],
                gain: 0.0,
                q: 1.0,
                typeName: "BELL"
            })
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
        hpfHz = 20
        lpfHz = 20000
        hpType = "HP LR 24"
        lpType = "LP LR 24"
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

    onBandCountChanged: rebuild()
    Component.onCompleted: rebuild()
}
