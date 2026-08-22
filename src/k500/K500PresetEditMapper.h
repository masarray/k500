#pragma once

#include "K500PresetCodec.h"

#include <QString>
#include <QVariant>

namespace K500PresetEditMapper {

struct EditResult {
    bool supported = false;
    K500PresetCodec::PatchResult patch;
};

// Translate one canonical StudioEngine stateEdited(path,value) event into an
// explicit byte whitelist. Unsupported paths are non-destructive and return
// supported=false. No field is normalized outside the requested edit.
EditResult applyEngineEdit(const QByteArray &source,
                           const QString &path,
                           const QVariant &value);

} // namespace K500PresetEditMapper
