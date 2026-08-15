#include "K500WinIo.h"

#include "K500Frame.h"

#include <QTimer>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <QWinEventNotifier>

#include <algorithm>
#include <vector>
#endif

class K500WinIo::Impl
{
public:
    explicit Impl(K500WinIo *owner)
        : q(owner)
    {
    }

    K500WinIo *q = nullptr;
    Kind kind = Kind::None;
    QString label;

#ifdef Q_OS_WIN
    HANDLE handle = INVALID_HANDLE_VALUE;

    std::unique_ptr<QTimer> serialPoll;

    HANDLE hidReadEvent = nullptr;
    OVERLAPPED hidReadOverlapped{};
    std::unique_ptr<QWinEventNotifier> hidReadNotifier;
    QByteArray hidReadBuffer;
    int hidInputReportLength = 65;
    int hidOutputReportLength = 65;
    bool closing = false;

    void emitWinError(const QString &prefix, DWORD code = GetLastError())
    {
        wchar_t *message = nullptr;
        const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
                          | FORMAT_MESSAGE_FROM_SYSTEM
                          | FORMAT_MESSAGE_IGNORE_INSERTS;
        FormatMessageW(flags, nullptr, code, 0,
                       reinterpret_cast<wchar_t *>(&message), 0, nullptr);
        QString detail = message ? QString::fromWCharArray(message).trimmed()
                                 : QStringLiteral("Windows error %1").arg(code);
        if (message)
            LocalFree(message);
        emit q->errorOccurred(QStringLiteral("%1: %2").arg(prefix, detail));
    }

    void pollSerial()
    {
        if (kind != Kind::Serial || handle == INVALID_HANDLE_VALUE)
            return;

        DWORD errors = 0;
        COMSTAT stat{};
        if (!ClearCommError(handle, &errors, &stat)) {
            emitWinError(QStringLiteral("Serial status failed"));
            return;
        }
        if (stat.cbInQue == 0)
            return;

        QByteArray data(static_cast<int>(qMin<DWORD>(stat.cbInQue, 4096)), Qt::Uninitialized);
        DWORD read = 0;
        if (!ReadFile(handle, data.data(), static_cast<DWORD>(data.size()), &read, nullptr)) {
            emitWinError(QStringLiteral("Serial read failed"));
            return;
        }
        if (read == 0)
            return;
        data.resize(static_cast<int>(read));
        emit q->bytesReceived(data);
    }

    void deliverHidBytes(DWORD bytesRead)
    {
        if (bytesRead == 0)
            return;
        QByteArray data = hidReadBuffer.left(static_cast<int>(bytesRead));

        // Windows HID user-mode buffers normally include report id 0 as byte
        // zero. Strip it only when the following byte is a K500 frame header.
        if (data.size() > 1 && static_cast<unsigned char>(data.at(0)) == 0x00) {
            const quint8 next = static_cast<quint8>(static_cast<unsigned char>(data.at(1)));
            if (next == 0x55 || next == 0xAA)
                data.remove(0, 1);
        }
        emit q->bytesReceived(data);
    }

    void issueHidRead()
    {
        if (closing || kind != Kind::UsbHid || handle == INVALID_HANDLE_VALUE)
            return;

        hidReadBuffer.resize(qMax(2, hidInputReportLength));
        hidReadBuffer.fill(char(0));
        ResetEvent(hidReadEvent);
        ZeroMemory(&hidReadOverlapped, sizeof(hidReadOverlapped));
        hidReadOverlapped.hEvent = hidReadEvent;

        DWORD bytesRead = 0;
        const BOOL ok = ReadFile(handle, hidReadBuffer.data(),
                                 static_cast<DWORD>(hidReadBuffer.size()),
                                 &bytesRead, &hidReadOverlapped);
        if (ok) {
            deliverHidBytes(bytesRead);
            QTimer::singleShot(0, q, [this] { issueHidRead(); });
            return;
        }

        const DWORD code = GetLastError();
        if (code == ERROR_IO_PENDING) {
            if (hidReadNotifier)
                hidReadNotifier->setEnabled(true);
            return;
        }
        if (!closing)
            emitWinError(QStringLiteral("USB HID read failed"), code);
    }

    void completeHidRead()
    {
        if (closing || handle == INVALID_HANDLE_VALUE)
            return;
        if (hidReadNotifier)
            hidReadNotifier->setEnabled(false);

        DWORD bytesRead = 0;
        if (!GetOverlappedResult(handle, &hidReadOverlapped, &bytesRead, FALSE)) {
            const DWORD code = GetLastError();
            if (code != ERROR_OPERATION_ABORTED && !closing)
                emitWinError(QStringLiteral("USB HID read completion failed"), code);
            return;
        }
        deliverHidBytes(bytesRead);
        issueHidRead();
    }

    bool writeOverlapped(const QByteArray &bytes, QString *error)
    {
        HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!event) {
            if (error)
                *error = QStringLiteral("Cannot create HID write event");
            return false;
        }

        OVERLAPPED ov{};
        ov.hEvent = event;
        DWORD written = 0;
        BOOL ok = WriteFile(handle, bytes.constData(), static_cast<DWORD>(bytes.size()),
                            &written, &ov);
        if (!ok && GetLastError() == ERROR_IO_PENDING) {
            const DWORD wait = WaitForSingleObject(event, 1200);
            if (wait == WAIT_OBJECT_0)
                ok = GetOverlappedResult(handle, &ov, &written, FALSE);
            else {
                CancelIoEx(handle, &ov);
                ok = FALSE;
                SetLastError(wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError());
            }
        }

        const DWORD code = ok ? ERROR_SUCCESS : GetLastError();
        CloseHandle(event);
        if (!ok || written != static_cast<DWORD>(bytes.size())) {
            if (error)
                *error = QStringLiteral("USB HID write failed (Windows error %1)").arg(code);
            return false;
        }
        return true;
    }
#endif
};

K500WinIo::K500WinIo(QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>(this))
{
}

K500WinIo::~K500WinIo()
{
    close();
}

QStringList K500WinIo::serialPorts()
{
    QStringList ports;
#ifdef Q_OS_WIN
    std::vector<wchar_t> buffer(65536, L'\0');
    const DWORD length = QueryDosDeviceW(nullptr, buffer.data(),
                                         static_cast<DWORD>(buffer.size()));
    if (length == 0)
        return ports;

    const wchar_t *cursor = buffer.data();
    while (*cursor) {
        const QString name = QString::fromWCharArray(cursor);
        if (name.startsWith(QStringLiteral("COM"), Qt::CaseInsensitive)) {
            bool numeric = false;
            name.mid(3).toInt(&numeric);
            if (numeric)
                ports.append(name.toUpper());
        }
        cursor += wcslen(cursor) + 1;
    }

    std::sort(ports.begin(), ports.end(), [](const QString &a, const QString &b) {
        return a.mid(3).toInt() < b.mid(3).toInt();
    });
#endif
    return ports;
}

bool K500WinIo::openSerial(const QString &portName, QString *error)
{
    close();
#ifdef Q_OS_WIN
    const QString path = QStringLiteral("\\\\.\\%1").arg(portName);
    HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(path.utf16()),
                                GENERIC_READ | GENERIC_WRITE,
                                0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        if (error)
            *error = QStringLiteral("Cannot open %1 (Windows error %2)")
                         .arg(portName).arg(GetLastError());
        return false;
    }

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(handle, &dcb)) {
        const DWORD code = GetLastError();
        CloseHandle(handle);
        if (error)
            *error = QStringLiteral("GetCommState %1 failed (%2)").arg(portName).arg(code);
        return false;
    }

    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!SetCommState(handle, &dcb)) {
        const DWORD code = GetLastError();
        CloseHandle(handle);
        if (error)
            *error = QStringLiteral("SetCommState %1 failed (%2)").arg(portName).arg(code);
        return false;
    }

    SetupComm(handle, 4096, 4096);
    PurgeComm(handle, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 900;
    SetCommTimeouts(handle, &timeouts);

    d->handle = handle;
    d->kind = Kind::Serial;
    d->label = portName.toUpper();
    d->serialPoll = std::make_unique<QTimer>(this);
    d->serialPoll->setInterval(15);
    connect(d->serialPoll.get(), &QTimer::timeout, this, [this] { d->pollSerial(); });
    d->serialPoll->start();
    return true;
#else
    Q_UNUSED(portName)
    if (error)
        *error = QStringLiteral("Native K500 transport currently requires Windows");
    return false;
#endif
}

bool K500WinIo::openUsbHid(quint16 vendorId, quint16 productId,
                           QString *deviceLabel, QString *error)
{
    close();
#ifdef Q_OS_WIN
    GUID hidGuid{};
    HidD_GetHidGuid(&hidGuid);
    HDEVINFO devices = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr,
                                            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devices == INVALID_HANDLE_VALUE) {
        if (error)
            *error = QStringLiteral("Cannot enumerate HID devices");
        return false;
    }

    QString matchedPath;
    for (DWORD index = 0;; ++index) {
        SP_DEVICE_INTERFACE_DATA iface{};
        iface.cbSize = sizeof(iface);
        if (!SetupDiEnumDeviceInterfaces(devices, nullptr, &hidGuid, index, &iface)) {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
                break;
            continue;
        }

        DWORD required = 0;
        SetupDiGetDeviceInterfaceDetailW(devices, &iface, nullptr, 0, &required, nullptr);
        if (required == 0)
            continue;

        std::vector<unsigned char> storage(required);
        auto *detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(storage.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(devices, &iface, detail, required,
                                              nullptr, nullptr))
            continue;

        HANDLE probe = CreateFileW(detail->DevicePath, 0,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr, OPEN_EXISTING, 0, nullptr);
        if (probe == INVALID_HANDLE_VALUE)
            continue;

        HIDD_ATTRIBUTES attributes{};
        attributes.Size = sizeof(attributes);
        const bool match = HidD_GetAttributes(probe, &attributes)
                        && attributes.VendorID == vendorId
                        && attributes.ProductID == productId;
        CloseHandle(probe);
        if (match) {
            matchedPath = QString::fromWCharArray(detail->DevicePath);
            break;
        }
    }
    SetupDiDestroyDeviceInfoList(devices);

    if (matchedPath.isEmpty()) {
        if (error)
            *error = QStringLiteral("USB HID DSP AUDIO %1:%2 not found")
                         .arg(vendorId, 4, 16, QLatin1Char('0'))
                         .arg(productId, 4, 16, QLatin1Char('0')).toUpper();
        return false;
    }

    HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(matchedPath.utf16()),
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        if (error)
            *error = QStringLiteral("K500 USB HID is present but cannot be opened. Close the original K500 app and retry. Windows error %1")
                         .arg(GetLastError());
        return false;
    }

    QString label = QStringLiteral("USB HID DSP AUDIO");
    wchar_t product[256]{};
    if (HidD_GetProductString(handle, product, sizeof(product))) {
        const QString detected = QString::fromWCharArray(product).trimmed();
        if (!detected.isEmpty())
            label = detected;
    }

    PHIDP_PREPARSED_DATA preparsed = nullptr;
    HIDP_CAPS caps{};
    if (HidD_GetPreparsedData(handle, &preparsed)) {
        if (HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS) {
            d->hidInputReportLength = qMax<int>(2, caps.InputReportByteLength);
            d->hidOutputReportLength = qMax<int>(2, caps.OutputReportByteLength);
        }
        HidD_FreePreparsedData(preparsed);
    }

    d->handle = handle;
    d->kind = Kind::UsbHid;
    d->label = label;
    d->closing = false;
    HidD_SetNumInputBuffers(handle, 64);

    d->hidReadEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!d->hidReadEvent) {
        const DWORD code = GetLastError();
        close();
        if (error)
            *error = QStringLiteral("Cannot create HID read event (%1)").arg(code);
        return false;
    }

    d->hidReadNotifier = std::make_unique<QWinEventNotifier>(d->hidReadEvent, this);
    d->hidReadNotifier->setEnabled(false);
    connect(d->hidReadNotifier.get(), &QWinEventNotifier::activated,
            this, [this] { d->completeHidRead(); });
    d->issueHidRead();

    if (deviceLabel)
        *deviceLabel = label;
    return true;
#else
    Q_UNUSED(vendorId)
    Q_UNUSED(productId)
    Q_UNUSED(deviceLabel)
    if (error)
        *error = QStringLiteral("Native K500 transport currently requires Windows");
    return false;
#endif
}

void K500WinIo::close()
{
#ifdef Q_OS_WIN
    d->closing = true;
    if (d->serialPoll) {
        d->serialPoll->stop();
        d->serialPoll.reset();
    }
    if (d->hidReadNotifier) {
        d->hidReadNotifier->setEnabled(false);
        d->hidReadNotifier.reset();
    }
    if (d->handle != INVALID_HANDLE_VALUE) {
        if (d->kind == Kind::UsbHid)
            CancelIoEx(d->handle, nullptr);
        CloseHandle(d->handle);
        d->handle = INVALID_HANDLE_VALUE;
    }
    if (d->hidReadEvent) {
        CloseHandle(d->hidReadEvent);
        d->hidReadEvent = nullptr;
    }
    ZeroMemory(&d->hidReadOverlapped, sizeof(d->hidReadOverlapped));
#endif
    d->kind = Kind::None;
    d->label.clear();
#ifdef Q_OS_WIN
    d->closing = false;
#endif
}

bool K500WinIo::isOpen() const
{
#ifdef Q_OS_WIN
    return d->handle != INVALID_HANDLE_VALUE && d->kind != Kind::None;
#else
    return false;
#endif
}

K500WinIo::Kind K500WinIo::kind() const
{
    return d->kind;
}

QString K500WinIo::label() const
{
    return d->label;
}

bool K500WinIo::writeProtocolFrame(const QByteArray &btFrame, QString *error)
{
#ifdef Q_OS_WIN
    if (!isOpen()) {
        if (error)
            *error = QStringLiteral("K500 transport is not open");
        return false;
    }

    if (d->kind == Kind::Serial) {
        DWORD written = 0;
        const BOOL ok = WriteFile(d->handle, btFrame.constData(),
                                  static_cast<DWORD>(btFrame.size()), &written, nullptr);
        if (!ok || written != static_cast<DWORD>(btFrame.size())) {
            if (error)
                *error = QStringLiteral("Serial write failed on %1 (Windows error %2)")
                             .arg(d->label).arg(GetLastError());
            return false;
        }
        return true;
    }

    const QByteArray usbFrame = K500Frame::toUsbFrame(btFrame);
    const int reportLength = qMax(2, d->hidOutputReportLength);
    const int payloadCapacity = reportLength - 1; // byte 0 is report id 0
    if (payloadCapacity <= 0) {
        if (error)
            *error = QStringLiteral("Invalid HID output report length");
        return false;
    }

    for (int offset = 0; offset < usbFrame.size(); offset += payloadCapacity) {
        QByteArray report(reportLength, char(0x00));
        const int count = qMin(payloadCapacity, usbFrame.size() - offset);
        std::copy_n(usbFrame.constData() + offset, count, report.data() + 1);
        if (!d->writeOverlapped(report, error))
            return false;
    }
    return true;
#else
    Q_UNUSED(btFrame)
    if (error)
        *error = QStringLiteral("Native K500 transport currently requires Windows");
    return false;
#endif
}
