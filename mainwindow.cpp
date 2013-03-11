#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QTextBlock>
#include <QFileDialog>
#include <QTimer>
#include <QSettings>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),ui(new Ui::MainWindow)
    {
    ui->setupUi(this);
    m_grblpos = 0; m_doupdate = false; grblset.nkomma = 3;
    m_curtab = 0;
    QString lcdcolor="color : #ff8000; background-color : #000000";
    ui->lcdMXpos->setStyleSheet(lcdcolor); ui->lcdMYpos->setStyleSheet(lcdcolor); ui->lcdMZpos->setStyleSheet(lcdcolor);
    ui->lcdWXpos->setStyleSheet(lcdcolor); ui->lcdWYpos->setStyleSheet(lcdcolor); ui->lcdWZpos->setStyleSheet(lcdcolor);
    ListPorts(m_coms);
    ui->grblcom->blockSignals(true);
    ui->keypadcom->blockSignals(true);
    for(int i=0; i<m_coms.size(); i++)
        {
        ui->grblcom->addItem(m_coms[i]);
        ui->keypadcom->addItem(m_coms[i]);
        }
    QObject::connect(&m_grblport,SIGNAL(InChar(int)),this,SLOT(InCharGRBL(int)));
    QObject::connect(&m_keypadport,SIGNAL(InChar(int)),this,SLOT(InCharKeyPad(int)));
    QSettings   qs("Joachim Koopmann Software","GRBLfront");
    if(m_coms.size() > 0)
        {
        int     i=qs.value("grblport",m_coms[0].toLocal8Bit().mid(3)).toInt();
        if(!m_grblport.Init(i,GRBLBAUD))
            QMessageBox::warning(this,"GRBLfront",QString("GRBL COM port %1 not available").arg(i));
        }
    if(m_coms.size() > 1)
        {
        int     i=qs.value("keypadport",m_coms[1].toLocal8Bit().mid(3)).toInt();
        if(!m_keypadport.Init(i,KEYPADBAUD))
            QMessageBox::warning(this,"GRBLfront",QString("KeyPad COM port %1 not available").arg(i));
        }
    if(m_grblport.m_com_port)
        {
        int     i=m_coms.indexOf(QString("COM%1").arg(m_grblport.m_com_port));
        if(i >= 0) ui->grblcom->setCurrentIndex(i);
        m_grblport.Transmitt("$$\n");
        }
    if(m_keypadport.m_com_port)
        {
        int     i=m_coms.indexOf(QString("COM%1").arg(m_keypadport.m_com_port));
        if(i >= 0) ui->keypadcom->setCurrentIndex(i);
        }
    ui->keypadcom->blockSignals(false);
    ui->grblcom->blockSignals(false);
    restoreGeometry(qs.value("geometry").toByteArray());

    QTimer *timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(update_state()));
    timer->start(50);
    }

void MainWindow::closeEvent(QCloseEvent *event)
    {
    QSettings   qs("Joachim Koopmann Software","GRBLfront");

    if(m_grblport.m_com_port) qs.setValue("grblport",m_grblport.m_com_port);
    if(m_keypadport.m_com_port) qs.setValue("keypadport",m_keypadport.m_com_port);
    qs.setValue("geometry",saveGeometry());
    event->accept();
    }

MainWindow::~MainWindow()
    {
    delete ui;
    }

void MainWindow::on_grblcom_currentIndexChanged(int index)
    {m_grblport.Init(m_coms[index].toLocal8Bit().mid(3).toInt(),GRBLBAUD);}

void MainWindow::on_keypadcom_currentIndexChanged(int index)
    {m_keypadport.Init(m_coms[index].toLocal8Bit().mid(3).toInt(),KEYPADBAUD);}

void MainWindow::on_tabWidget_currentChanged(int index)
    {m_curtab = index;}

void MainWindow::InCharGRBL(int chr)
    {
    m_grbllock.lock();
    m_grblbuf[m_grblpos++] = chr;
    m_grblpos &= 0xFF; m_grblbuf[m_grblpos] = 0;
    if(chr == '\n')
        {
        QString	qs = QString::fromLocal8Bit(m_grblbuf,m_grblpos).trimmed();
        if(qs[0] == '<') ui->stateEdit->setText(qs); else AddResponse(qs);
        DecodeResponse(qs);
        m_grblpos = 0;
        }
    m_grbllock.unlock();
    }

void MainWindow::InCharKeyPad(int chr)
    {
    bool    idle = (grblstate == "Idle");

    m_keypadlock.lock();
    chr &= 0xFF;
    switch(chr)
        {
        case 0x80:	if(idle) emit on_homeButton_clicked(); break;		// REF down
        case 0x81:	if(idle) emit on_gonullButton_clicked(); break;  	// NullPos down
        case 0x82:	if(idle) emit on_gotoollButton_clicked(); break;	// ParkPos down
        case 0x84:	if(idle) emit jog(UPLEFT); break;       			// UPLEFT down
        case 0x04:	emit jogend(); break;                           	// UPLEFT up
        case 0x85:	if(idle) emit jog(UP); break;       				// UP down
        case 0x05:	emit jogend(); break;                           	// UP up
        case 0x86:	if(idle) emit jog(UPRIGHT); break;          		// UPRIGHT down
        case 0x06:	emit jogend(); break;                               // UPRIGHT up
        case 0x87:	if(idle) emit jog(ZUP); break;                      // ZUP down
        case 0x07:	emit jogend(); break;                               // ZUP up
        case 0x88:	if(idle) emit jog(LEFT); break;                     // LEFT down
        case 0x08:	emit jogend(); break;                               // LEFT up
        case 0x89:	if(idle) emit on_xynullButton_clicked(); break; 	// Set XY Null down
        case 0x8A:	if(idle) emit jog(RIGHT); break;                    // RIGHT down
        case 0x0A:	emit jogend(); break;                               // RIGHT up
        case 0x8B:	if(idle) emit on_znullButton_clicked(); break;		// Set Z Null down
        case 0x8C:	if(idle) emit jog(DOWNLEFT); break;                 // DOWNLEFT down
        case 0x0C:	emit jogend(); break;                               // DOWNLEFT up
        case 0x8D:	if(idle) emit jog(DOWN); break;                     // DOWN down
        case 0x0D:	emit jogend(); break;                               // DOWN up
        case 0x8E:	if(idle) emit jog(DOWNRIGHT); break;                // DOWNRIGHT down
        case 0x0E:	emit jogend(); break;                               // DOWNRIGHT up
        case 0x8F:	if(idle) emit jog(ZDOWN); break;                    // ZDOWN down
        case 0x0F:	emit jogend(); break;                               // ZDOWN up
        case 0x90:	shift = true; break;								// SHIFT down
        case 0x10:	shift = false; break;								// SHIFT up
        case 0x91:	emit cmdchar(shift?'~':'!'); break;         		// PAUSE down
        case 0x92:	emit on_gsendButton_clicked(); break;   			// START down
        case 0x93:	emit on_resetButton_clicked(); break;				// STOP down
        }
    m_keypadlock.unlock();
    }

void MainWindow::update_state()
    {
    if(0 == m_curtab) m_grblport.Transmitt("?");
    if(kill && (grblstate == "Queue"))
        {
        kill = false; m_grblport.Transmitt("%");
        }
    }

void MainWindow::AddResponse(QString& qs)
    {
    QTextDocument *doc = ui->actionEdit->document();

    ui->actionEdit->append(qs);
    QTextBlock block = doc->begin();
    while(doc->lineCount() > RLINE)
        {
        QTextCursor cursor(block);
        block = block.next();
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        }
    }

void MainWindow::DecodeResponse(QString& qt)
    {
    if(qt.compare("OK",Qt::CaseInsensitive) == 0)
        {
        QTextDocument   *doc = ui->gcodeEdit->document();
        QTextBlock      li;

        if(m_doupdate) {ShowSettings(); m_doupdate = false;}
        if(m_transfer.size())
            {
            m_grblport.Transmitt(m_transfer[0].toLocal8Bit()); m_transfer.removeAt(0);
            return;
            }
        for(;;)
            {
            if(curline >= maxline) return;
            li = doc->findBlockByNumber(curline);
            curline++;
            if(li.text()[0] != '(') break;
            }
        QTextCursor cu(li);
        cu.select(QTextCursor::LineUnderCursor);
        ui->gcodeEdit->setTextCursor(cu);
        AddResponse(li.text());
        m_grblport.Transmitt(li.text().toLocal8Bit().data());
        m_grblport.Transmitt("\n");
        ui->glView->update();
        }
    else if(qt[0] == '$')
        {
        int         i;

        QRegExp blanks("['\1'-' ']");
        qt.remove(blanks);
        if(qt.left(3) == "$0=") grblset.xsmm = readFloat(qt.mid(3));
        if(qt.left(3) == "$1=") grblset.ysmm = readFloat(qt.mid(3));
        if(qt.left(3) == "$2=") grblset.zsmm = readFloat(qt.mid(3));
        if(qt.left(3) == "$3=") grblset.xvmax = readFloat(qt.mid(3)) / 60.;
        if(qt.left(3) == "$4=") grblset.yvmax = readFloat(qt.mid(3)) / 60.;
        if(qt.left(3) == "$5=") grblset.zvmax = readFloat(qt.mid(3)) / 60.;
        if(qt.left(3) == "$6=") grblset.xacc = readFloat(qt.mid(3));
        if(qt.left(3) == "$7=") grblset.yacc = readFloat(qt.mid(3));
        if(qt.left(3) == "$8=") grblset.zacc = readFloat(qt.mid(3));
        if(qt.left(3) == "$9=") grblset.plen = readInt(qt.mid(3));
        if(qt.left(4) == "$10=") grblset.feed = readFloat(qt.mid(4)) / 60.;
        if(qt.left(4) == "$11=")
            {
            i = readInt(qt.mid(4));
            grblset.xstep = (i & 4) ? true : false;
            grblset.ystep = (i & 8) ? true : false;
            grblset.zstep = (i & 16) ? true : false;
            grblset.xdir = (i & 32) ? true : false;
            grblset.ydir = (i & 64) ? true : false;
            grblset.zdir = (i & 128) ? true : false;
            }
        if(qt.left(4) == "$12=") grblset.stepidle = readInt(qt.mid(4));
        if(qt.left(4) == "$13=") grblset.dev = readFloat(qt.mid(4));
        if(qt.left(4) == "$14=") grblset.arctol = readFloat(qt.mid(4));
        if(qt.left(4) == "$15=") grblset.nkomma = readInt(qt.mid(4));
        if(qt.left(4) == "$16=") grblset.inch = readInt(qt.mid(4)) ? true : false;
        if(qt.left(4) == "$17=") grblset.autostart = readInt(qt.mid(4)) ? true : false;
        if(qt.left(4) == "$18=") grblset.invstep = readInt(qt.mid(4)) ? true : false;
        if(qt.left(4) == "$19=") grblset.hardlimit = readInt(qt.mid(4)) ? true : false;
        if(qt.left(4) == "$20=") grblset.homecycle = readInt(qt.mid(4)) ? true : false;
        if(qt.left(4) == "$21=")
            {
            i = readInt(qt.mid(4));
            grblset.xhome = (i & 32) ? true : false;
            grblset.yhome = (i & 64) ? true : false;
            grblset.zhome = (i & 128) ? true : false;
            }
        if(qt.left(4) == "$22=") grblset.homefeed = readFloat(qt.mid(4)) / 60.;
        if(qt.left(4) == "$23=") grblset.homeseek = readFloat(qt.mid(4)) / 60.;
        if(qt.left(4) == "$24=") grblset.homedebounce = readInt(qt.mid(4));
        if(qt.left(4) == "$25=") grblset.homepulloff = readFloat(qt.mid(4));
        m_doupdate = true;
        }
    // <Idle,MPos:0.000,0.000,0.000,WPos:0.000,0.000,0.000>
    else if(qt[0] == '<')
        {
        qt.remove(QRegExp("[<>]"));
        QStringList ql=qt.split(',',QString::KeepEmptyParts);
        if(ql.length() == 7 && ql[1].left(5) == "MPos:" && ql[4].left(5) == "WPos:")
            {
            grblstate = ql[0];
            grblpos.mposx = ql[1].mid(5).toDouble();
            grblpos.mposy = ql[2].toDouble();
            grblpos.mposz = ql[3].toDouble();
            grblpos.wposx = ql[4].mid(5).toDouble();
            grblpos.wposy = ql[5].toDouble();
            grblpos.wposz = ql[6].toDouble();
            char    buf[16];
            sprintf_s(buf,16,"%.2f",grblpos.mposx); ui->lcdMXpos->display(buf);
            sprintf_s(buf,16,"%.2f",grblpos.mposy); ui->lcdMYpos->display(buf);
            sprintf_s(buf,16,"%.2f",grblpos.mposz); ui->lcdMZpos->display(buf);
            sprintf_s(buf,16,"%.2f",grblpos.wposx); ui->lcdWXpos->display(buf);
            sprintf_s(buf,16,"%.2f",grblpos.wposy); ui->lcdWYpos->display(buf);
            sprintf_s(buf,16,"%.2f",grblpos.wposz); ui->lcdWZpos->display(buf);
            QPixmap pm(ui->stateView->width(),ui->stateView->height());
            if(grblstate == "Idle") pm.fill(QColor(0,255,0));
            else if(grblstate == "Queue") pm.fill(QColor(0,255,128));
            else if(grblstate == "Run") pm.fill(QColor(255,192,0));
            else if(grblstate == "Alarm") pm.fill(QColor(255,0,0));
            else pm.fill(QColor(128,128,128));
            ui->stateView->setPixmap(pm);
            }
        }
    }

void MainWindow::ShowSettings()
    {
    char    buf[32];
    int     n=grblset.nkomma;

    sprintf_s(buf,32,"%.*f",n,grblset.xsmm); ui->xsmmEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.ysmm); ui->ysmmEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.zsmm); ui->zsmmEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.xvmax); ui->xmaxEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.yvmax); ui->ymaxEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.zvmax); ui->zmaxEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.xacc); ui->xaccEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.yacc); ui->yaccEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.zacc); ui->zaccEdit->setText(buf);
    ui->xstepBox->setChecked(grblset.xstep);
    ui->ystepBox->setChecked(grblset.ystep);
    ui->zstepBox->setChecked(grblset.zstep);
    ui->xdirBox->setChecked(grblset.xdir);
    ui->ydirBox->setChecked(grblset.ydir);
    ui->zdirBox->setChecked(grblset.zdir);
    ui->xhomeBox->setChecked(grblset.xhome);
    ui->yhomeBox->setChecked(grblset.yhome);
    ui->zhomeBox->setChecked(grblset.zhome);
    ui->inchBox->setChecked(grblset.inch);
    ui->hardlimitBox->setChecked(grblset.hardlimit);
    ui->homecycleBox->setChecked(grblset.homecycle);
    ui->autostartBox->setChecked(grblset.autostart);
    ui->invstepBox->setChecked(grblset.invstep);
    sprintf_s(buf,32,"%.*f",n,grblset.homeseek); ui->homeseekEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.homefeed); ui->homefeedEdit->setText(buf);
    sprintf_s(buf,32,"%d",grblset.plen); ui->plenEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.feed); ui->feedEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.dev); ui->devEdit->setText(buf);
    sprintf_s(buf,32,"%.*f",n,grblset.arctol); ui->arctolEdit->setText(buf);
    sprintf_s(buf,32,"%d",grblset.nkomma); ui->nkommaEdit->setText(buf);
    }

void MainWindow::GetSettings()
    {
    grblset.xsmm = ui->xsmmEdit->text().toFloat();
    grblset.ysmm = ui->ysmmEdit->text().toFloat();
    grblset.zsmm = ui->zsmmEdit->text().toFloat();
    grblset.xvmax = ui->xmaxEdit->text().toFloat();
    grblset.yvmax = ui->ymaxEdit->text().toFloat();
    grblset.zvmax = ui->zmaxEdit->text().toFloat();
    grblset.xacc = ui->xaccEdit->text().toFloat();
    grblset.yacc = ui->yaccEdit->text().toFloat();
    grblset.zacc = ui->zaccEdit->text().toFloat();
    grblset.xstep = ui->xstepBox->isChecked();
    grblset.ystep = ui->ystepBox->isChecked();
    grblset.zstep = ui->zstepBox->isChecked();
    grblset.xdir = ui->xdirBox->isChecked();
    grblset.ydir = ui->ydirBox->isChecked();
    grblset.zdir = ui->zdirBox->isChecked();
    grblset.xhome = ui->xhomeBox->isChecked();
    grblset.yhome = ui->yhomeBox->isChecked();
    grblset.zhome = ui->zhomeBox->isChecked();
    grblset.inch = ui->inchBox->isChecked();
    grblset.hardlimit = ui->hardlimitBox->isChecked();
    grblset.homecycle = ui->homecycleBox->isChecked();
    grblset.autostart = ui->autostartBox->isChecked();
    grblset.invstep = ui->invstepBox->isChecked();
    grblset.homeseek = ui->homeseekEdit->text().toFloat();
    grblset.homefeed = ui->homefeedEdit->text().toFloat();
    grblset.plen = ui->plenEdit->text().toInt();
    grblset.feed = ui->feedEdit->text().toFloat();
    grblset.dev = ui->devEdit->text().toFloat();
    grblset.arctol = ui->arctolEdit->text().toFloat();
    grblset.nkomma = ui->nkommaEdit->text().toInt();
    }

void MainWindow::jog(JOGDIR dir)
    {
    char    buf[256];
    double	xdist=0.,ydist=0.,zdist=0.,dist=1000.;
    int		n=grblset.nkomma;

    switch(dir)
        {
        case UP:		ydist = dist; break;
        case UPRIGHT:	xdist = dist; ydist = dist; break;
        case RIGHT:		xdist = dist; break;
        case DOWNRIGHT:	xdist = dist; ydist = -dist; break;
        case DOWN:		ydist = -dist; break;
        case DOWNLEFT:	xdist = -dist; ydist = -dist; break;
        case LEFT:		xdist = -dist; break;
        case UPLEFT:	xdist = -dist; ydist = dist; break;
        case ZUP:		zdist = dist; break;
        case ZDOWN:		zdist = -dist; break;
        }
    if(shift) sprintf_s(buf,256,"G91\nG0 X%.*f Y%.*f Z%.*f\nG90\n",n,xdist,n,ydist,n,zdist);
    else sprintf_s(buf,256,"G91\nG1 X%.*f Y%.*f Z%.*f F%.*f\nG90\n",n,xdist,n,ydist,n,zdist,n,0.1*60.);
    moving = true; m_grblport.Transmitt(buf);
    }

void MainWindow::jogend()
    {
    if(moving)
        {
        m_grblport.Transmitt("!"); moving = false; kill = true;
        }
    }

void MainWindow::cmdchar(int chr)
    {
    char    buf[4];

    sprintf_s(buf,4,"%c",chr); m_grblport.Transmitt(buf);
    }

void MainWindow::StartTransfer()
    {
    if(m_transfer.isEmpty()) return;
    m_grblport.Transmitt(m_transfer[0].toLocal8Bit()); m_transfer.removeAt(0);
    }

void MainWindow::on_gloadButton_clicked()
    {
    QString name = QFileDialog::getOpenFileName(this,"G-Code laden","","G-Code (*.tap *.nc)");
    if(name.isEmpty()) return;
    QApplication::setOverrideCursor(Qt::WaitCursor); QApplication::processEvents();

    STEPCOORvec	sta;
    if(GRBLsim(name,sta,1.) > 0)
        {
        CalcPoints(sta);
        ui->glView->Init(); ui->glView->updateGL();
        }

    QFile   *fi = new QFile(name);
    qint64  l;
    char    *buf=0;
    try
        {
        if(!fi->open(QFile::ReadOnly | QFile::Text)) throw 0;
        l = fi->size();
        if(!(buf = new char[l+1])) throw 0;
        l = fi->read(buf,l);
        fi->close(); buf[l] = 0; ui->gcodeEdit->setText(buf);
        sprintf_s(buf,l,"Loaded %d G-code lines in %d blocks",
            ui->gcodeEdit->document()->lineCount(),blocks.size());
        AddResponse(QString(buf));
        }
    catch(...) {}
    delete []buf; delete fi;
    QApplication::setOverrideCursor(Qt::ArrowCursor);
    }

void MainWindow::on_gsendButton_clicked()
    {
    QTextDocument   *doc = ui->gcodeEdit->document();

    if(doc->lineCount() <= 1) return;
    curline = 0; maxline = doc->lineCount(); DecodeResponse(QString("ok"));
    }

void MainWindow::on_resetButton_clicked()
    {
    QTextDocument   *doc = ui->gcodeEdit->document();
    char            res[2]={0x18,0};

    if(doc->lineCount() > 0) m_grblport.Transmitt("!");
    m_grblport.Transmitt(res);
    if(doc->lineCount() > 0) {Sleep(1000); m_grblport.Transmitt("$X\n");}
    curline = 0; maxline = 0; ui->glView->update();
    }

void MainWindow::on_homeButton_clicked()
    {
    m_grblport.Transmitt("$H\n");
    }

void MainWindow::on_checkButton_clicked()
    {
    m_grblport.Transmitt("$C\n");
    }

void MainWindow::on_xynullButton_clicked()
    {
    char	buf[_MAX_PATH];

    if(grblstate == "Alarm") return;
    sprintf_s(buf,_MAX_PATH,"G10 L2 P1 X%.*f Y%.*f\n",
        grblset.nkomma,grblpos.mposx,grblset.nkomma,grblpos.mposy);
    m_grblport.Transmitt(buf);
    }

void MainWindow::on_znullButton_clicked()
    {
    char	buf[_MAX_PATH];

    m_transfer.empty();
    if(grblstate == "Alarm") return;
    sprintf_s(buf,_MAX_PATH,"G10 L2 P1 Z%.*f\n",grblset.nkomma,grblpos.mposz);
    m_transfer.append(buf);
    sprintf_s(buf,_MAX_PATH,"G91\nG0 Z%.*f\nG90\n",grblset.nkomma,zsecure*(grblset.zdir?-1.:1.));
    m_transfer.append(buf);
    StartTransfer();
    }

void MainWindow::on_gonullButton_clicked()
    {
    char	buf[_MAX_PATH];

    if(grblstate == "Alarm") return;
    sprintf_s(buf,_MAX_PATH,"G0 X0 Y0 Z%.*f\n",grblset.nkomma,zsecure*(grblset.zdir?-1.:1.));
    m_grblport.Transmitt(buf);
    }

void MainWindow::on_paramreadButton_clicked()
    {
    m_grblport.Transmitt("$$\n");
    }

void MainWindow::on_paramwriteButton_clicked()
    {
    GetSettings(); FormatSettings(m_transfer); StartTransfer();
    }

void MainWindow::on_paramloadButton_clicked()
    {
    QString line,name = QFileDialog::getOpenFileName(this,"Parameter laden","","Parameter (*.prm)");
    if(name.isEmpty()) return;
    QFile   fi(name);
    if(!fi.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream st(&fi);
    for(;;)
        {
        line = fi.readLine();
        if(line.isEmpty()) break;
        DecodeResponse(line);
        }
    fi.close();
    ShowSettings(); m_doupdate = false;
    }

void MainWindow::on_paramsaveButton_clicked()
    {
    int         i,l;
    QStringList ca;
    QString     name;

    name = QFileDialog::getSaveFileName(this,"Parameter speichern","","Parameter (*.prm)");
    if(name.isEmpty()) return;
    l = FormatSettings(ca);
    QFile   fi(name);
    if(!fi.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream st(&fi);
    for(i=0; i<l; i++) st << ca[i];
    fi.close();
    }

void MainWindow::on_gotoollButton_clicked()
    {
    if(grblstate == "Alarm") return;
    m_transfer.empty(); m_transfer.append("G53 G0 Z0\n");
    m_transfer.append("G53 G0 X0 Y0\n"); StartTransfer();
    }
