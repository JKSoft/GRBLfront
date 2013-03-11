#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <vector>
#include <QString>
#include <QStringList>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include "globals.h"

GRBLsettings    grblset={0};
GRBLpos         grblpos={0};
QString         grblstate;
FCoorvec		gpoints;
DWordvec		blocks;
int             curline=0,maxline=0;
double          zsecure=-5.;
bool            shift=false,moving=false,kill=false;

bool ListPorts(QStringList& ports)
    {
    bool    bSuccess=false;
    HKEY    hSERIALCOMM;
    char    *valueName=0,*valueData=0;

    ports.clear();
    if(RegOpenKeyExA(HKEY_LOCAL_MACHINE,"HARDWARE\\DEVICEMAP\\SERIALCOMM",0,KEY_QUERY_VALUE,&hSERIALCOMM) == ERROR_SUCCESS)
        {
        DWORD dwMaxValueNameLen;
        DWORD dwMaxValueLen;
        DWORD dwQueryInfo = RegQueryInfoKeyA(hSERIALCOMM,0,0,0,0,0,0,0,&dwMaxValueNameLen,&dwMaxValueLen,0,0);
        if(dwQueryInfo == ERROR_SUCCESS)
            {
            //Allocate some space for the value name and value data
            dwMaxValueNameLen++; //Include space for the NULL terminator
            if((valueName = new char[dwMaxValueNameLen]) && (valueData = new char[dwMaxValueLen]))
                {
                bSuccess = true;
                //Enumerate all the values underneath HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM
                DWORD dwIndex = 0;
                DWORD dwType;
                DWORD dwValueNameSize = dwMaxValueNameLen;
                DWORD dwDataSize = dwMaxValueLen;
                memset(valueName,0,dwMaxValueNameLen);
                memset(valueData,0,dwMaxValueLen);
                LONG nEnum = RegEnumValueA(hSERIALCOMM,dwIndex,valueName,&dwValueNameSize,0,&dwType,(LPBYTE)valueData,&dwDataSize);
                while(nEnum == ERROR_SUCCESS)
                    {
                    //If the value is of the correct type, then add it to the array
                    if(dwType == REG_SZ)
                        {
                        valueData[dwDataSize] = 0;
                        ports.append(QString::fromLocal8Bit(valueData));
                        }
                    //Prepare for the next time around
                    dwValueNameSize = dwMaxValueNameLen;
                    dwDataSize = dwMaxValueLen;
                    memset(valueName,0,dwMaxValueNameLen);
                    memset(valueData,0,dwMaxValueLen);
                    ++dwIndex;
                    nEnum = RegEnumValueA(hSERIALCOMM,dwIndex,valueName,&dwValueNameSize,0,&dwType,(LPBYTE)valueData,&dwDataSize);
                    }
                }
            }
        RegCloseKey(hSERIALCOMM);
        }
    delete []valueData; delete []valueName;
    return bSuccess;
    }

// grbl_sim.exe 0.01 <HelloWorld.nc >HelloWorld.dat 2> HelloWorldSteps.dat
int GRBLsim(QString& gcode, STEPCOORvec& ca, double reso)
    {
    QString             home=QApplication::applicationDirPath();
    QString             tmp=QDir::tempPath();
    char                cmd[_MAX_PATH*5],buf[_MAX_PATH];
    QFileInfo           qfi(gcode);
    QString             bl,st,gc;
    STARTUPINFOA		si;
    PROCESS_INFORMATION	pi;
    FILE				*fi=0,*fo=0;
    QStringList         sca;
    STEPCOOR			stc={0,0,0,0};
    double              time;
    int                 i,l;

    home += "/grbl_sim.exe";
    if(!QFile::exists(home)) {QMessageBox::warning(0,"GRBLfront","grbl_sim.exe required"); return 0;}
    bl = tmp + "/grblsimblocks.dat";
    st = tmp + "/grblsimsteps.dat";
    gc = tmp + "/" + qfi.fileName();
    try
        {
        if(!(fo = fopen(gc.toLocal8Bit(),"wt"))) throw 0;
        if(!(fi = fopen(gcode.toLocal8Bit(),"rt"))) throw 0;
        l = FormatSettings(sca);
        for(i=0; i<l; i++) fwrite(sca[i].toLocal8Bit().data(),1,sca[i].length(),fo);
        while(!feof(fi))
            {
            l = fread(cmd,1,_MAX_PATH*5,fi);
            if(0 == l) break;
            fwrite(cmd,1,l,fo);
            }
        fclose(fi); fi = 0; fclose(fo); fo = 0;
        // call grbl_sim
        sprintf_s(cmd,_MAX_PATH*5,"cmd /C \"\"%s\" %f <\"%s\" >\"%s\" 2> \"%s\"\"",
            home.toLocal8Bit().data(),reso,gc.toLocal8Bit().data(),bl.toLocal8Bit().data(),st.toLocal8Bit().data());
        ZeroMemory(&si,sizeof(si)); si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
        ZeroMemory(&pi,sizeof(pi));
        if(QFile::exists(st)) QFile::remove(st);
        if(!CreateProcessA(0,cmd,0,0,FALSE,0,0,0,&si,&pi)) throw 0;
        WaitForSingleObject(pi.hProcess,INFINITE);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
        if(!QFile::exists(st)) throw 0;
        // read step coordinates
        if(!(fi = fopen(st.toLocal8Bit(),"rt"))) throw 0;
        for(;;)
            {
            cmd[0] = 0; fgets(cmd,_MAX_PATH,fi); if(feof(fi)) break;
            if(0 == cmd[0] || '\r' == cmd[0]) continue;
            if(strncmp(cmd,"# block number",14) == 0) {stc.blockcnt = atoi(&cmd[15]); continue;}
            if(sscanf(cmd,"%lf,%ld,%ld,%ld",&time,&stc.x,&stc.y,&stc.z) != 4) continue;
            ca.push_back(stc);
            }
        fclose(fi); fi = 0;
        // read block info
        if(!(fi = fopen(bl.toLocal8Bit(),"rt"))) throw 0;
        blocks.clear(); cmd[0] = 0;
        for(stc.blockcnt=0;;)
            {
            strcpy_s(buf,_MAX_PATH,cmd);
            cmd[0] = 0; fgets(cmd,_MAX_PATH,fi); if(feof(fi)) break;
            if(0 == cmd[0]) continue;
            if(cmd[0] != '#') {stc.blockcnt++; continue;}
            if(strncmp(cmd,"# ok",4) == 0 && strncmp(buf,"# $",3) != 0) blocks.push_back(stc.blockcnt);
            }
        fclose(fi); fi = 0;
        }
    catch(...) {ca.clear();}
    if(fi) fclose(fi); if(fo) fclose(fo);
    if(QFile::exists(st)) QFile::remove(st);
    if(QFile::exists(bl)) QFile::remove(bl);
    if(QFile::exists(gc)) QFile::remove(gc);
    return ca.size();
    }

int FormatSettings(QStringList& ca)
    {
    int		i,n=grblset.nkomma;
    char	buf[32];

    ca.clear();
    sprintf_s(buf,32,"$0=%.*f\n",n,grblset.xsmm); ca.push_back(buf);
    sprintf_s(buf,32,"$1=%.*f\n",n,grblset.ysmm); ca.push_back(buf);
    sprintf_s(buf,32,"$2=%.*f\n",n,grblset.zsmm); ca.push_back(buf);
    sprintf_s(buf,32,"$3=%.*f\n",n,grblset.xvmax*60.); ca.push_back(buf);
    sprintf_s(buf,32,"$4=%.*f\n",n,grblset.yvmax*60.); ca.push_back(buf);
    sprintf_s(buf,32,"$5=%.*f\n",n,grblset.zvmax*60.); ca.push_back(buf);
    sprintf_s(buf,32,"$6=%.*f\n",n,grblset.xacc); ca.push_back(buf);
    sprintf_s(buf,32,"$7=%.*f\n",n,grblset.yacc); ca.push_back(buf);
    sprintf_s(buf,32,"$8=%.*f\n",n,grblset.zacc); ca.push_back(buf);
    sprintf_s(buf,32,"$9=%d\n",grblset.plen); ca.push_back(buf);
    sprintf_s(buf,32,"$10=%.*f\n",n,grblset.feed*60.); ca.push_back(buf);
    i = 0;
    if(grblset.xstep) i |= 4;
    if(grblset.ystep) i |= 8;
    if(grblset.zstep) i |= 16;
    if(grblset.xdir) i |= 32;
    if(grblset.ydir) i |= 64;
    if(grblset.zdir) i |= 128;
    sprintf_s(buf,32,"$11=%d\n",i); ca.push_back(buf);
    sprintf_s(buf,32,"$12=%d\n",grblset.stepidle); ca.push_back(buf);
    sprintf_s(buf,32,"$13=%.*f\n",n,grblset.dev); ca.push_back(buf);
    sprintf_s(buf,32,"$14=%.*f\n",n,grblset.arctol); ca.push_back(buf);
    sprintf_s(buf,32,"$15=%d\n",n); ca.push_back(buf);
    i = grblset.inch ? 1 : 0; sprintf_s(buf,32,"$16=%d\n",i); ca.push_back(buf);
    i = grblset.autostart ? 1 : 0; sprintf_s(buf,32,"$17=%d\n",i); ca.push_back(buf);
    i = grblset.invstep ? 1 : 0; sprintf_s(buf,32,"$18=%d\n",i); ca.push_back(buf);
    i = grblset.hardlimit ? 1 : 0; sprintf_s(buf,32,"$19=%d\n",i); ca.push_back(buf);
    i = grblset.homecycle ? 1 : 0; sprintf_s(buf,32,"$20=%d\n",i); ca.push_back(buf);
    i = 0;
    if(grblset.xhome) i |= 32;
    if(grblset.yhome) i |= 64;
    if(grblset.zhome) i |= 128;
    sprintf_s(buf,32,"$21=%d\n",i); ca.push_back(buf);
    sprintf_s(buf,32,"$22=%.*f\n",n,grblset.homefeed*60.); ca.push_back(buf);
    sprintf_s(buf,32,"$23=%.*f\n",n,grblset.homeseek*60.); ca.push_back(buf);
    sprintf_s(buf,32,"$24=%d\n",grblset.homedebounce); ca.push_back(buf);
    sprintf_s(buf,32,"$25=%.*f\n",n,grblset.homepulloff); ca.push_back(buf);
    return ca.size();
    }

float readFloat(QString& qt)
    {
    int i=qt.indexOf('(');
    if(i > 0) qt.truncate(i);
    return qt.toFloat();
    }

int readInt(QString& qt)
    {
    int i=qt.indexOf('(');
    if(i > 0) qt.truncate(i);
    return qt.toInt();
    }

void GetDimension(STEPCOORvec &ca, int& xsize, int& ysize, int& zsize)
    {
    int		xmin=INT_MAX,xmax=INT_MIN;
    int		ymin=INT_MAX,ymax=INT_MIN;
    int		zmin=INT_MAX,zmax=INT_MIN;
    int		i,l=ca.size();

    for(i=0; i<l; i++)
        {
        if(ca[i].x < xmin) xmin = ca[i].x; if(ca[i].x > xmax) xmax = ca[i].x;
        if(ca[i].y < ymin) ymin = ca[i].y; if(ca[i].y > ymax) ymax = ca[i].y;
        if(ca[i].z < zmin) zmin = ca[i].z; if(ca[i].z > zmax) zmax = ca[i].z;
        }
    xsize = xmax - xmin; ysize = ymax - ymin; zsize = zmax - zmin;
    for(i=0; i<l; i++)
        {
        ca[i].x -= xmin; ca[i].y -= ymin; ca[i].z -= zmin;
        }
    }

void CalcPoints(STEPCOORvec &ca)
    {
    int		i,l=ca.size();
    int		xs,ys,zs;
    float	xf,yf,zf;
    FCoor   fc;

    GetDimension(ca,xs,ys,zs);
    xf = 1.f / (float)xs; yf = 1.f / (float)ys; zf = 1.f / (float)zs;
    xf = __min(__min(xf,yf),zf);
    gpoints.clear();
    for(i=0; i<l; i++)
        {
        if(i > 0 && ca[i].x == ca[i-1].x && ca[i].y == ca[i-1].y && ca[i].z == ca[i-1].z) continue;
        fc.blockcnt = ca[i].blockcnt;
        fc.x = (float)ca[i].x * xf - .5f;
        fc.y = (float)ca[i].y * xf - .5f;
        fc.z = (float)ca[i].z * xf;
        gpoints.push_back(fc);
        }
    }
