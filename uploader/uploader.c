#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#define USAGE_MSG       "Usage: [port] [firmware]\n"
#define PORT_ERR_MSG    "Unable to open port!\n"
#define FILE_ERR_MSG    "Unable to open file!\n"
#define DONE_MSG        "Upload completed successfully!\n"
#define CRYPTO_ERR_MSG  "Upload failed!\n"
#define BAD_RESP_MSG    "Invalid response from hardware!\n"
#define WAITING_MSG     "Waiting for response...\n"
#define GOT_RESP_MSG    "Starting upload...\n"
#define WRONG_RESP_MSG  "Wrong response byte!\n"

void byteToHex( char *arr, unsigned char byte )
{
    char alphabet[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    arr[1] = alphabet[ byte % 16 ];
    arr[0] = alphabet[ ( byte / 16 ) % 16 ];
}

int port;
char readByte()
{
    char byte = 0;
    char echo[7] = "R:   \n";
    read( port, &byte, 1 );
    byteToHex( &echo[3], byte );
    write( STDOUT_FILENO, echo, 6 );
    return byte;
}

char sendByte( char byte )
{
    char echo[7] = "W:    ";
    byteToHex( &echo[3], byte );
    write( port, &byte, 1 );
    write( STDOUT_FILENO, echo, 6 );
}

int main( int argc, char *argv[] )
{
    int file;
    int file_size = 0;
    char fbyte, byte;
    char echoing[4];
    struct termios options;

    echoing[2] = ' ';
    echoing[3] = '\0';

    if( argc != 3 ) {
        write( STDERR_FILENO, USAGE_MSG, sizeof(USAGE_MSG) );
        return -1;
    }

    port = open( argv[1], O_RDWR | O_NOCTTY );

    if( port <= 0 ) {
        write( STDERR_FILENO, PORT_ERR_MSG, sizeof(PORT_ERR_MSG) );
        return -1;
    }

    file = open( argv[2], O_RDONLY );

    if( port <= 0 ) {
        write( STDERR_FILENO, FILE_ERR_MSG, sizeof(FILE_ERR_MSG) );
        return -1;
    }

    tcgetattr( port, &options );

    cfsetispeed( &options, B9600 );
    cfsetospeed( &options, B9600 );

    options.c_cflag |= ( CLOCAL | CREAD );

    tcsetattr( port, TCSANOW, &options );

    write( STDOUT_FILENO, WAITING_MSG, sizeof( WAITING_MSG ) );

    byte = readByte();

    if( byte == (char) 0xC0 ) {
        byte = 0x60;
        sendByte( byte );
    } else {
        write( STDOUT_FILENO, WRONG_RESP_MSG, sizeof( WRONG_RESP_MSG ) );
        return -1;
    }

    write( STDOUT_FILENO, GOT_RESP_MSG, sizeof( GOT_RESP_MSG ) );

    while( read( file, &fbyte, 1 ) > 0 ) {
        byte = readByte();

        if( byte == (char) 0xC0 ) {
            sendByte( fbyte );
        } else if( byte == (char) 0x0C ) {
            write( STDOUT_FILENO, DONE_MSG, sizeof( DONE_MSG ) );
            break;
        } else if( byte == (char) 0xFF ) {
            write( STDERR_FILENO, CRYPTO_ERR_MSG, sizeof( CRYPTO_ERR_MSG ) );
            break;
        } else {
            write( STDERR_FILENO, BAD_RESP_MSG, sizeof( BAD_RESP_MSG ) );
            break;
        }
    }

    close( port );
    close( file );

    return 0;
}

