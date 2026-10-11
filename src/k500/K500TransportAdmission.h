#pragma once

// PRESET_EXCLUSIVE_TX_POLICY_V1 — pure, testable transport admission.
// A Save preflight may dispatch already-authorized LIVE slider writes,
// but heartbeat/player/background traffic is forbidden from the moment
// a preset operation becomes busy. Once the exclusive preset transaction
// starts, only the preset coordinator can transmit.
namespace K500TransportAdmission {
enum class Owner { Runtime, Live, Preset };

constexpr bool permits(bool presetBusy, bool savePreflight, Owner owner)
{
    if (!presetBusy)
        return owner != Owner::Preset;
    if (owner == Owner::Preset)
        return !savePreflight;
    return owner == Owner::Live && savePreflight;
}
} // namespace K500TransportAdmission
