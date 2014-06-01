#include "intelhexprocessor.h"

intelHexProcessor::intelHexProcessor()
{
}

QByteArray intelHexProcessor::processDataLine(QByteArray line, int *address, int word_size, int address_length, bool *ok)
{
    QByteArray output;
    output.clear();

    int position = line.indexOf( ':' );
    if( position == -1 ) {
        if( ok != NULL ) *ok = false;
        return output;
    } else position += 1;

    int byte_count = line.mid( position, 2 ).toInt( NULL, 16 );
    position += 2;

    if( address != NULL ) {
        *address =  line.mid( position, address_length ).toUInt( NULL, 16 );
    }
    position += address_length;

    if( line.mid(position, 2).toInt() != 0 ) {
        if( ok != NULL ) *ok = false;
        return output;
    } else position += 2;

    for( int i = 0; i < byte_count; i++, position += 2 ) {
        output.append( (unsigned char) line.mid( position, 2 ).toInt( NULL, 16 ) );
    }

    if( ok != NULL ) *ok = true;
    return output;
}
