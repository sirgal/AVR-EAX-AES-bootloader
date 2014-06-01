#ifndef INTELHEXPROCESSOR_H
#define INTELHEXPROCESSOR_H

#include <uchar.h>
#include <QString>
#include <QByteArray>
#include <QDebug>

using namespace std;

class intelHexProcessor
{
public:
    intelHexProcessor();
    QByteArray processDataLine(QByteArray line,
            int *address = NULL,
            int word_size = 2,
            int address_length = 4,
            bool *ok = NULL
            );
};

#endif // INTELHEXPROCESSOR_H
