#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QFileDialog>
#include "intelhexprocessor.h"
#include "eaxencrypt.h"
#include <QMessageBox>
#include <QDateTime>

#include "osrng.h"
using CryptoPP::AutoSeededRandomPool;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public slots:
    void generateKeyFile();
    void openKeyFile(QString in_filename = QString() );
    void encryptHex();
    void fixValues();
signals:

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;
    intelHexProcessor parser;
    eaxEncrypt cryptard;
    byte *header;

    QString pretty_AVR_block_format( QString name, QByteArray data );

    bool key_open;
    void selftest();
};

#endif // MAINWINDOW_H
