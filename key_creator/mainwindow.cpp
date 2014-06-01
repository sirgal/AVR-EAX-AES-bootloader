#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    const char const_header[] = "ConstantFlorence";
    key_open = false;

    ui->setupUi(this);

    header = new byte[AES::BLOCKSIZE];
    memcpy( header, const_header, AES::BLOCKSIZE );

    adjustSize();
    setFixedSize( size() );

    connect( ui->encryptButt, SIGNAL(clicked()), this, SLOT(encryptHex()) );
    connect( ui->openKeyButt, SIGNAL(clicked()), this, SLOT(openKeyFile()) );
    connect( ui->genKeyButt,  SIGNAL(clicked()), this, SLOT(generateKeyFile()) );
    connect( ui->flashSizeBox,SIGNAL(editingFinished()), this, SLOT(fixValues()));
    connect( ui->pageSizeBox, SIGNAL(editingFinished()), this, SLOT(fixValues()));

    selftest();
}

MainWindow::~MainWindow()
{
    delete header;
    delete ui;
}

void MainWindow::openKeyFile( QString in_filename )
{
    QFile key_file;
    QString filename;
    QString asm_filter( "Assembly key file (*.asm)" );

    if( in_filename.isEmpty() ) {
        filename = QFileDialog::getOpenFileName( this, "Select key file to open", "", asm_filter, &asm_filter );
    } else filename = in_filename;

    key_file.setFileName( filename );
    if( !key_file.open( QFile::ReadOnly )) {
        QMessageBox::warning( this, "Opening failed!", "Failed to open file for reading" );
        return;
    }

    QString current_line;
    bool keys_found = false;
    while( !key_file.atEnd() ) {
        current_line = key_file.readLine();
        if( current_line.indexOf( "Key UID: " ) != -1 ) {
            ui->uidLabel->setText( current_line.remove( ";" ).remove( "\n" ) );
        }
        if( current_line.indexOf( "KEYS:" ) != -1 ) {
            keys_found = true;
            break;
        }
    }
    if( keys_found ) {
        current_line = key_file.readLine(); //.db \
        current_line = key_file.readLine(); //key data
        current_line = key_file.readLine(); //key data

        current_line.remove( "0x" );
        current_line.remove( ", " );

        QByteArray key( current_line.toUtf8() );
        key = key.fromHex(key);

        cryptard.setKey( (byte*) key.data(), header );
        key_open = true;
    } else {
        QMessageBox::warning( this, "Invalid key file!", "Unable to locate KEYS field in given file." );
    }
}

void MainWindow::generateKeyFile()
{
    QFile key_file;
    QString asm_filter( "Assembly key file (*.asm)" );

    QString filename = QFileDialog::getSaveFileName( this, "Select file to write key to", "", asm_filter, &asm_filter );
    if( filename.isEmpty() ) return;

    key_file.setFileName( filename );
    if( !key_file.open( QFile::WriteOnly ) ) {
        QMessageBox::warning( this, "Opening failed!", "Failed to open file for writing" );
        return;
    }

    AutoSeededRandomPool *prng;
    prng = new AutoSeededRandomPool;

    byte new_key[AES::BLOCKSIZE];
    byte UID[AES::BLOCKSIZE];

    QByteArray file_buffer;
    file_buffer.append( ";BOOTLOADER KEY FILE, INCLUDES PRECOMPUTED VALUES. SOFTWARE-GENERATED, DO NOT CHANGE\n" );
    file_buffer.append( ";FOR USE WITH EAX AUTHENTICATED ENCRYPTION MODE\n" );
    file_buffer.append( ";GENERATION TIME: " + QDateTime::currentDateTimeUtc().toString() + "\n" );
    prng->GenerateBlock( UID, AES::BLOCKSIZE );
    file_buffer.append( ";Key UID: " + QString(QByteArray( (char*)  UID, AES::BLOCKSIZE ).toHex()).toUpper() + "\n\n" );

    prng->GenerateBlock( new_key, AES::BLOCKSIZE );
    cryptard.setKey( new_key, header );

    delete prng;
    file_buffer.append( ".org KEY_ADDR\n" );

    file_buffer.append( pretty_AVR_block_format( "KEYS",
                                                 cryptard.key_expansion() ) );
    file_buffer.append( pretty_AVR_block_format( "PRECOMP_L0",
                                                 QByteArray( (char*) cryptard.L0, AES::BLOCKSIZE ) ) );
    file_buffer.append( pretty_AVR_block_format( "PRECOMP_L2",
                                                 QByteArray( (char*) cryptard.L2, AES::BLOCKSIZE ) ) );
    file_buffer.append( pretty_AVR_block_format( "PRECOMP_B",
                                                 QByteArray( (char*) cryptard.B,  AES::BLOCKSIZE ) ) );
    file_buffer.append( pretty_AVR_block_format( "PRECOMP_HEADER_TAG",
                                                 QByteArray( (char*) cryptard.header, AES::BLOCKSIZE ) ) );

    key_file.write( file_buffer );
    key_file.close();

    openKeyFile( filename );
    key_open = true;
}

void MainWindow::encryptHex()
{
    const int boot_size = 1024;
    int flash_size = ui->flashSizeBox->value();
    int page_size  = ui->pageSizeBox->value();
    int firmware_size = flash_size - boot_size;
    if( !ui->useZeroPageBox->isChecked() ) firmware_size -= page_size;

    QFile file_in, file_out;
    QByteArray firmware;

    QString hex_filter( "Intel HEX file (*.hex)" );
    QString bin_filter( "Binary file (*.bin)"    );

    if( !key_open ) openKeyFile();
    if( !key_open ) return;

    QString filename_in = QFileDialog::getOpenFileName( this, "Select Intel HEX file to encrypt", "", hex_filter, &hex_filter );
    if( filename_in.isEmpty() ) return;
    QString filename_out= QFileDialog::getSaveFileName( this, "Select output file", "", bin_filter, &bin_filter );
    if( filename_out.isEmpty() ) return;

    file_in.setFileName( filename_in );
    file_out.setFileName( filename_out );

    if( !file_in.open( QFile::ReadOnly ) ) {
        QMessageBox::warning( this, "Opening failed!", "Failed to open file for reading" );
        return;
    }

    if( !file_out.open( QFile::WriteOnly )
        && !filename_out.isEmpty() ) {
        QMessageBox::warning( this, "Opening failed!", "Failed to open file for writing" );
        return;
    }

    byte *temp;
    AutoSeededRandomPool *prng;

    temp = new byte[firmware_size];
    prng = new AutoSeededRandomPool( true );

    prng->GenerateBlock( temp, firmware_size );
    firmware.setRawData( (char*) temp, firmware_size );

    delete temp;
    delete prng;

    int address;
    QByteArray current_block, current_line;
    bool parse_ok;

    while( !file_in.atEnd() ) {
        current_line = file_in.readLine();
        current_line.chop(1);
        current_block = parser.processDataLine( current_line, &address, 2, 4, &parse_ok );

        if( parse_ok && address < (flash_size - boot_size) ) {
            if( !ui->useZeroPageBox->isChecked() ) {
                if( address < page_size )
                    continue;
                address -= page_size;
            }

            firmware.replace( address, current_block.length(), current_block );
        }
    }

    QByteArray file_buffer;
    for( int i = 0; i < (firmware_size / page_size); i++ ) {
        file_buffer.append( cryptard.encrypt( firmware.mid( i*page_size, page_size ) ) );
    }

    file_out.write( file_buffer );

    file_in.close();
    file_out.close();
}

QString MainWindow::pretty_AVR_block_format(QString name, QByteArray data)
{
    QString output;

    output += name.toUpper() + ":\n";
    output += ".db ";

    for( int i = 0, j = 0; i < data.length(); i++, j++ ) {
        if( j % 16 == 0 ) output.append( "\\\n" );
        output.append( QString("0x") + QString( "%1, ").arg( (byte) data[i], 2, 16, QChar('0') ).toUpper() );
    }
    output.chop( 2 );
    output.append( "\n\n" );

    return output;
}

void MainWindow::selftest()
{
    byte key[] = { 0x6C, 0xCD, 0xFB, 0x58, 0x51, 0xFE, 0xD9, 0x7B, 0x96, 0x60, 0xD8, 0x38, 0xEE, 0x45, 0x5E, 0x22 };
    byte test_msg[] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
    QByteArray message( (char*) test_msg);
    cryptard.setKey( key, header );

    QByteArray output = cryptard.encrypt( message );
}

void MainWindow::fixValues()
{
    QSpinBox *sender = qobject_cast<QSpinBox*>( QObject::sender() );
    int difference = sender->value() % 16;

    if( difference!= 0 ) {
        if( difference < 8 ) {
            sender->setValue( sender->value() - difference );
        } else if( difference >= 8 ) {
            sender->setValue( sender->value() - difference + 16 );
        }
    }
}
