#pragma once

#include <QtGlobal>

struct RuntimeHealthSnapshot
{
    qint64 workingSetBytes = -1;
    qint64 privateBytes = -1;
    qint64 peakWorkingSetBytes = -1;
    quint32 handleCount = 0;
    quint32 threadCount = 0;
    bool valid = false;
};

class RuntimeMetrics final
{
public:
    RuntimeMetrics() = delete;

    static RuntimeHealthSnapshot capture();
};
