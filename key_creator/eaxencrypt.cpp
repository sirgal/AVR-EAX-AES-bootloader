#include "eaxencrypt.h"

eaxEncrypt::eaxEncrypt()
{
    memset( key, 0, AES::BLOCKSIZE );
    memset( L0,  0, AES::BLOCKSIZE );
    memset( L1,  0, AES::BLOCKSIZE );
    memset( L2,  0, AES::BLOCKSIZE );
    memset( B,   0, AES::BLOCKSIZE );
}

QByteArray eaxEncrypt::encrypt( QByteArray data_block )
{
    QByteArray output;

    byte Tag[AES::BLOCKSIZE] = {0};
    byte nonce[AES::BLOCKSIZE] = {0};

    byte N[AES::BLOCKSIZE] = {0};
    byte H[AES::BLOCKSIZE] = {0};
    byte C[AES::BLOCKSIZE] = {0};

    byte *temp_array;
    byte *Ciphertext;
    Ciphertext = new byte[data_block.length()];

    CTR_Mode<AES>::Encryption ctr_encrypt;
    AutoSeededRandomPool prng;

    //NONCE
    prng.GenerateBlock( nonce, AES::BLOCKSIZE );
    output.append( (char*) nonce, AES::BLOCKSIZE );
    //N
    temp_array = OMAC( L0, nonce, AES::BLOCKSIZE );
    memcpy( N, temp_array, AES::BLOCKSIZE );
    //H (precomputed once during key setup)
    memcpy( H, header, AES::BLOCKSIZE );
    //encipher with CTR
    ctr_encrypt.SetKeyWithIV( key, AES::BLOCKSIZE, N );
    ctr_encrypt.ProcessData( Ciphertext, (byte*) data_block.data(), data_block.length() );
    output.append( (char*) Ciphertext, data_block.length() );
    //C
    temp_array = OMAC( L2, Ciphertext, data_block.length() );
    memcpy( C, temp_array, AES::BLOCKSIZE );

    //TAG
    memset( Tag, 0, AES::BLOCKSIZE );
    xor_arrays( Tag, N );
    xor_arrays( Tag, H );
    xor_arrays( Tag, C );

    output.append( (char*) Tag, AES::BLOCKSIZE );

    delete Ciphertext;
    return output;
}

byte* eaxEncrypt::OMAC(byte tweak[], byte *data, int length)
{
    byte *temp;
    byte Tag[AES::BLOCKSIZE] = {0};
    temp = new byte[length];
    memcpy( temp, data, length );

    for( int i = length-1, j = AES::BLOCKSIZE-1; j >= 0; i--, j-- ) {
        temp[i] ^= B[j];
    }

    for( int i = 0; i < length/AES::BLOCKSIZE; i++ ) {
        xor_arrays( &temp[i*AES::BLOCKSIZE], (i == 0) ? tweak : Tag );
        cipher.ProcessData( Tag, &temp[i*AES::BLOCKSIZE], AES::BLOCKSIZE );
    }

    delete temp;
    return Tag;
}

void eaxEncrypt::xor_arrays( byte a[], byte b[] )
{
    for( int i = 0; i < AES::BLOCKSIZE; i++ ) {
        a[i] = a[i] ^ b[i];
    }
}

void eaxEncrypt::setKey( byte in_key[AES::BLOCKSIZE], byte in_header[] )
{
    byte temp_block[AES::BLOCKSIZE] = {0};
    memcpy( key, in_key, AES::BLOCKSIZE );
    cipher.SetKey( key, AES::BLOCKSIZE );

    temp_block[15] = 0;
    cipher.ProcessData( L0, temp_block, AES::BLOCKSIZE );
    temp_block[15] = 1;
    cipher.ProcessData( L1, temp_block, AES::BLOCKSIZE );
    temp_block[15] = 2;
    cipher.ProcessData( L2, temp_block, AES::BLOCKSIZE );
    cipher.ProcessData( temp_block, key, AES::BLOCKSIZE );

    memcpy( B, L0, AES::BLOCKSIZE );
    bool curr_carry = false;
    bool last_carry = false;
    for( int i = 15; i >= 0; i-- ) {
        curr_carry = (B[i] & 0x80) != 0;
        B[i] <<= 1;
        B[i] |= last_carry ? 1 : 0;
        last_carry = curr_carry;
    }
    if( last_carry ) B[15] ^= 0x87;

    memcpy( header, OMAC( L1, in_header, AES::BLOCKSIZE ), AES::BLOCKSIZE );
}

void eaxEncrypt::print_block(byte block[], bool dollar)
{
    byte string[512] = {0};
    byte small_string[256] = {0};
    for( int i = 0; i < AES::BLOCKSIZE; i++ ) {
        if( dollar )
            sprintf( (char*) small_string, "$%02X", block[i] );
        else
            sprintf( (char*) small_string, "0x%02X, ", block[i] );
        strcat( (char*) string, (char*) small_string );
    }
    qDebug() << QString( (char*) string);
}

QByteArray eaxEncrypt::key_expansion()
{
    unsigned char Rcon[11] = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
    };
    const unsigned char s_box[256] =
    {
       0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
       0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
       0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
       0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
       0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
       0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
       0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
       0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
       0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
       0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
       0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
       0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
       0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
       0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
       0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
       0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
    };
    byte output[AES::BLOCKSIZE*11] = {0};
    memcpy( output, key, AES::BLOCKSIZE );
    for( int k = 1; k < 11; k++ ) {
        output[k*AES::BLOCKSIZE] = Rcon[k];

        for( int i = 0; i < 3; i++ ) {
            output[k*AES::BLOCKSIZE + i] ^= output[(k-1)*AES::BLOCKSIZE + i] ^ s_box[output[(k-1)*AES::BLOCKSIZE + 12 + i + 1]];
        }
        output[k*AES::BLOCKSIZE + 3] = output[(k-1)*AES::BLOCKSIZE + 3] ^ s_box[output[(k-1)*AES::BLOCKSIZE + 12]];

        for( int i = 4; i < 16; i++ ) {
            output[k*AES::BLOCKSIZE + i] = output[(k-1)*AES::BLOCKSIZE + i] ^ output[k*AES::BLOCKSIZE + i - 4];
        }
    }
    return QByteArray( (char*) output, AES::BLOCKSIZE*11 );
}
