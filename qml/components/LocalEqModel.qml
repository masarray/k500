import QtQuick

Item {
    id: root
    visible: false
    width: 0
    height: 0

    property int bandCount: 5
    property real hpfHz: 20
    property real lpfHz: 20000
    property string hpType: "HP LR 24"
    property string lpType: "LP LR 24"
    property var defaultFrequencies: []
    readonly property int count: bandList.count

    signal bandChanged()
    signal crossoverChanged()

    ListModel { id: bandList }

    function defaultsForCount() {
        if (defaultFrequencies && defaultFrequencies.length === bandCount)
            return defaultFrequencies
        if (bandCount === 10) return [80, 125, 250, 500, 1000, 2000, 4000, 6300, 10000, 12500]
        if (bandCount === 7) return [80, 160, 315, 630, 1250, 2500, 8000]
        return [80, 250, 800, 2500, 8000]
    }

    function rebuild() {
        bandList.clear()
        var freqs = defaultsForCount()
        for (var i = 0; i < bandCount; ++i) {
            bandList.append({
                freq: freqs[i],
                gain: 0.0,
                q: 1.0,
                typeName: "BELL"
            })
        }
        bandChanged()
    }

    function get(index) { return bandList.get(index) }

    function setBand(index, frequency, gain, q) {
        if (index < 0 || index >= bandList.count) return
        bandList.setProperty(index, "freq", Math.max(20, Math.min(20000, Number(frequency))))
        bandList.setProperty(index, "gain", Math.max(-24, Math.min(24, Number(gain))))
        bandList.setProperty(index, "q", Math.max(0.1, Math.min(30, Number(q))))
        bandChanged()
    }

    function setBandType(index, typeName) {
        if (index < 0 || index >= bandList.count) return
        bandList.setProperty(index, "typeName", typeName)
        bandChanged()
    }

    function resetBand(index) {
        if (index < 0 || index >= bandList.count) return
        var freqs = defaultsForCount()
        bandList.set(index, { freq: freqs[index], gain: 0.0, q: 1.0, typeName: "BELL" })
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
