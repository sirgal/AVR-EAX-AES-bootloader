#ifndef EAXENCRYPT_H
#define EAXENCRYPT_H

#include "osrng.h"
using CryptoPP::AutoSeededRandomPool;

#include "aes.h"
using CryptoPP::AES;

#include "modes.h"
using namespace CryptoPP;

#include <QByteArray>
#include <QDebug>
#include <stdio.h>

class eaxEncrypt
{
public:
    eaxEncrypt();

    void setKey(byte in_key[] , byte in_header[AES::BLOCKSIZE]);
    QByteArray encrypt( QByteArray data_block );

    byte L0[AES::BLOCKSIZE];
    byte L1[AES::BLOCKSIZE];
    byte L2[AES::BLOCKSIZE];
    byte B [AES::BLOCKSIZE];
    byte header[AES::BLOCKSIZE];

    QByteArray key_expansion();
private:
    byte key[AES::BLOCKSIZE];

    ECB_Mode<AES>::Encryption cipher;

    void xor_arrays(byte a[], byte b[]);
    void print_block( byte block[AES::BLOCKSIZE], bool dollar = false );
    byte* OMAC( byte tweak[AES::BLOCKSIZE], byte *data, int length );
};

#endif // EAXENCRYPT_H
