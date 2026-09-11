#pragma once

#include <QtGlobal>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <hidsdi.h>
#include <setupapi.h>

#include <utility>

// P3_WINDOWS_RAII_V1
// Small move-only guards for the native resources used by K500WinIo. They are
// intentionally dependency-free and keep the release operation beside the
// resource type so every early-return/error path can become mechanically safe.

class UniqueWinHandle final
{
public:
    UniqueWinHandle() = default;
    explicit UniqueWinHandle(HANDLE handle) noexcept : m_handle(handle) {}
    ~UniqueWinHandle() noexcept { reset(); }

    UniqueWinHandle(const UniqueWinHandle &) = delete;
    UniqueWinHandle &operator=(const UniqueWinHandle &) = delete;

    UniqueWinHandle(UniqueWinHandle &&other) noexcept
        : m_handle(other.release())
    {
    }

    UniqueWinHandle &operator=(UniqueWinHandle &&other) noexcept
    {
        if (this != &other)
            reset(other.release());
        return *this;
    }

    HANDLE get() const noexcept { return m_handle; }
    bool valid() const noexcept
    {
        return m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE;
    }
    explicit operator bool() const noexcept { return valid(); }

    HANDLE release() noexcept
    {
        HANDLE value = m_handle;
        m_handle = INVALID_HANDLE_VALUE;
        return value;
    }

    void reset(HANDLE replacement = INVALID_HANDLE_VALUE) noexcept
    {
        if (valid())
            CloseHandle(m_handle);
        m_handle = replacement;
    }

private:
    HANDLE m_handle = INVALID_HANDLE_VALUE;
};

class UniqueDeviceInfoSet final
{
public:
    UniqueDeviceInfoSet() = default;
    explicit UniqueDeviceInfoSet(HDEVINFO value) noexcept : m_value(value) {}
    ~UniqueDeviceInfoSet() noexcept { reset(); }

    UniqueDeviceInfoSet(const UniqueDeviceInfoSet &) = delete;
    UniqueDeviceInfoSet &operator=(const UniqueDeviceInfoSet &) = delete;

    UniqueDeviceInfoSet(UniqueDeviceInfoSet &&other) noexcept
        : m_value(other.release())
    {
    }

    UniqueDeviceInfoSet &operator=(UniqueDeviceInfoSet &&other) noexcept
    {
        if (this != &other)
            reset(other.release());
        return *this;
    }

    HDEVINFO get() const noexcept { return m_value; }
    bool valid() const noexcept { return m_value != INVALID_HANDLE_VALUE; }
    explicit operator bool() const noexcept { return valid(); }

    HDEVINFO release() noexcept
    {
        HDEVINFO value = m_value;
        m_value = INVALID_HANDLE_VALUE;
        return value;
    }

    void reset(HDEVINFO replacement = INVALID_HANDLE_VALUE) noexcept
    {
        if (valid())
            SetupDiDestroyDeviceInfoList(m_value);
        m_value = replacement;
    }

private:
    HDEVINFO m_value = INVALID_HANDLE_VALUE;
};

class UniqueHidPreparsedData final
{
public:
    UniqueHidPreparsedData() = default;
    explicit UniqueHidPreparsedData(PHIDP_PREPARSED_DATA value) noexcept : m_value(value) {}
    ~UniqueHidPreparsedData() noexcept { reset(); }

    UniqueHidPreparsedData(const UniqueHidPreparsedData &) = delete;
    UniqueHidPreparsedData &operator=(const UniqueHidPreparsedData &) = delete;

    UniqueHidPreparsedData(UniqueHidPreparsedData &&other) noexcept
        : m_value(other.release())
    {
    }

    UniqueHidPreparsedData &operator=(UniqueHidPreparsedData &&other) noexcept
    {
        if (this != &other)
            reset(other.release());
        return *this;
    }

    PHIDP_PREPARSED_DATA get() const noexcept { return m_value; }
    explicit operator bool() const noexcept { return m_value != nullptr; }

    PHIDP_PREPARSED_DATA release() noexcept
    {
        PHIDP_PREPARSED_DATA value = m_value;
        m_value = nullptr;
        return value;
    }

    void reset(PHIDP_PREPARSED_DATA replacement = nullptr) noexcept
    {
        if (m_value)
            HidD_FreePreparsedData(m_value);
        m_value = replacement;
    }

private:
    PHIDP_PREPARSED_DATA m_value = nullptr;
};

class UniqueLocalBuffer final
{
public:
    UniqueLocalBuffer() = default;
    explicit UniqueLocalBuffer(void *value) noexcept : m_value(value) {}
    ~UniqueLocalBuffer() noexcept { reset(); }

    UniqueLocalBuffer(const UniqueLocalBuffer &) = delete;
    UniqueLocalBuffer &operator=(const UniqueLocalBuffer &) = delete;

    UniqueLocalBuffer(UniqueLocalBuffer &&other) noexcept
        : m_value(other.release())
    {
    }

    UniqueLocalBuffer &operator=(UniqueLocalBuffer &&other) noexcept
    {
        if (this != &other)
            reset(other.release());
        return *this;
    }

    void *get() const noexcept { return m_value; }
    explicit operator bool() const noexcept { return m_value != nullptr; }

    void *release() noexcept
    {
        void *value = m_value;
        m_value = nullptr;
        return value;
    }

    void reset(void *replacement = nullptr) noexcept
    {
        if (m_value)
            LocalFree(m_value);
        m_value = replacement;
    }

private:
    void *m_value = nullptr;
};

#endif // Q_OS_WIN
