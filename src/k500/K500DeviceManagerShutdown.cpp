#include "K500DeviceManager.h"

#include "K500Controller.h"

// P3_DETERMINISTIC_SHUTDOWN_V1
// Keep teardown in a tiny translation unit so shutdown sequencing remains easy
// to audit independently from the protocol/readback state machine.

K500DeviceManager::~K500DeviceManager()
{
    shutdown();
}

void K500DeviceManager::shutdown()
{
    if (m_shuttingDown)
        return;
    m_shuttingDown = true;

    // 1) Stop producing/admitting live traffic at the controller boundary.
    if (m_controller) {
        disconnect(m_controller, &K500Controller::frameReady,
                   this, &K500DeviceManager::sendLiveFrame);
    }
    setLiveEnabled(false);

    // 2) Stop all manager-side timers/state machines before native I/O closes.
    m_responseTimer.stop();
    m_probeDelayTimer.stop();
    m_heartbeatTimer.stop();
    m_stage = Stage::Idle;
    m_parser.reset();

    if (m_controller)
        m_controller->clearDeviceState();

    m_activeMemory.clear();
    m_memoryReadOffset = 0;
    m_pendingReadLength = 0;
    m_muted = false;

    // 3) Worker-side shutdown performs CancelIoEx/completion, releases RAII
    // resources, quits the worker event loop and joins the thread.
    m_io.shutdown();
}
