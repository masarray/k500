.pragma library

// PEQ_CAPTION_SPATIAL_LAYOUT_V2 — pure geometry; no hardware/model writes.
// All input coordinates and measured Text dimensions are in graph pixels.
function intersects(a, b, gap) {
    return a.x < b.x + b.w + gap && a.x + a.w + gap > b.x
        && a.y < b.y + b.h + gap && a.y + a.h + gap > b.y
}

function place(o) {
    // VISUAL_SAFE_INSET_V4 — captions stay inside the plot's optical safe
    // area, away from rounded panel borders and nearby axis/grid labels.
    var edge = 12
    var minX = Math.max(edge, o.left + 12)
    var maxRight = Math.max(minX + 1, Math.min(o.width - edge, o.right - 12))
    var maxH = Math.max(o.hpH, o.lpH)
    var baseline = Math.max(edge, o.top + 10)
    // First try the graph's familiar guide row. Other lanes are selected only
    // when measured captions or the currently visible inspector obstruct it.
    var lanes = [
        {y: baseline, cost: 0},
        {y: edge, cost: 11},
        {y: edge + maxH + 7, cost: 14},
        {y: baseline + maxH + 7, cost: 32},
        {y: Math.max(edge, o.bottom - maxH - 12), cost: 72}
    ]
    function candidates(anchor, width, height, preferred) {
        var out = []
        for (var side = 0; side < 2; ++side) {
            var direction = side === 0 ? preferred : -preferred
            var rawX = anchor + (direction > 0 ? 10 : -10 - width)
            var x = Math.max(minX, Math.min(maxRight - width, rawX))
            // If the graph is extremely narrow, remain inside its viewport.
            x = Math.max(edge, Math.min(Math.max(edge, o.width - width - edge), x))
            for (var i = 0; i < lanes.length; ++i) {
                var row = lanes[i]
                if (row.y < edge || row.y + height > o.height - edge)
                    continue
                var rect = {x:x, y:row.y, w:width, h:height}
                var obstacle = o.obstacle
                if (obstacle && intersects(rect, obstacle, 4))
                    continue
                out.push({rect:rect, x:x, y:row.y,
                    score:row.cost + (side ? 4 : 0) + Math.abs(x - rawX) * 0.22})
            }
        }
        return out
    }
    var hp = candidates(o.hpX, o.hpW, o.hpH, 1)
    var lp = candidates(o.lpX, o.lpW, o.lpH, -1)
    var best = null
    for (var i = 0; i < hp.length; ++i) {
        for (var j = 0; j < lp.length; ++j) {
            if (intersects(hp[i].rect, lp[j].rect, 7))
                continue
            var score = hp[i].score + lp[j].score
            // Tie-break toward the physical left-to-right HP/LP ordering.
            if (hp[i].x > lp[j].x)
                score += 1
            if (!best || score < best.score)
                best = {hp:{x:hp[i].x,y:hp[i].y},
                        lp:{x:lp[j].x,y:lp[j].y}, score:score}
        }
    }
    if (best)
        return {hp:best.hp, lp:best.lp}

    // Tiny window fallback: preserve both labels, separated vertically.
    // Normal Windows minimum sizes always use the collision-free search above.
    var y1 = edge, y2 = Math.min(o.height - o.lpH - edge, y1 + o.hpH + 7)
    return {
        hp:{x:Math.max(minX, Math.min(maxRight-o.hpW, o.hpX+10)),y:y1},
        lp:{x:Math.max(minX, Math.min(maxRight-o.lpW, o.lpX+10)),y:y2}
    }
}
