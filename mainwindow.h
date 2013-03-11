#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMutex>
#include "serial.h"
#include "globals.h"

namespace Ui {
    class MainWindow;
    }

class MainWindow : public QMainWindow
    {
    Q_OBJECT
    
public:
    QStringList m_coms;
    CSerial     m_grblport,m_keypadport;
    char        m_grblbuf[256];
    int         m_grblpos;
    QMutex      m_grbllock,m_keypadlock;
    bool        m_doupdate;
    int         m_curtab;
    QStringList m_transfer;

explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
public slots:
    void InCharGRBL(int chr);
    void InCharKeyPad(int chr);

private slots:
    void on_grblcom_currentIndexChanged(int index);
    void update_state();
    void jog(JOGDIR dir);
    void jogend();
    void cmdchar(int chr);
    void on_gloadButton_clicked();
    void on_gsendButton_clicked();
    void on_resetButton_clicked();
    void on_homeButton_clicked();
    void on_checkButton_clicked();
    void on_xynullButton_clicked();
    void on_znullButton_clicked();
    void on_gonullButton_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_paramreadButton_clicked();
    void on_paramwriteButton_clicked();
    void on_keypadcom_currentIndexChanged(int index);
    void on_gotoollButton_clicked();
    void on_paramloadButton_clicked();
    void on_paramsaveButton_clicked();

private:
    Ui::MainWindow *ui;

    void    closeEvent(QCloseEvent *event);
    void    AddResponse(QString& qs);
    void    DecodeResponse(QString& txt);
    void    ShowSettings();
    void    GetSettings();
    void    StartTransfer();
    };

#endif // MAINWINDOW_H
