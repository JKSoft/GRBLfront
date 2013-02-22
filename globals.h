#ifndef GLOBALS_H
#define GLOBALS_H

#define GRBLBAUD    57600
#define KEYPADBAUD  115200
#define RLINE       100

typedef struct _GRBLsettings
    {
    float   xsmm;			// $0=250.000 (x, step/mm)
    float   ysmm;			// $1=250.000 (y, step/mm)
    float   zsmm;			// $2=250.000 (z, step/mm)
    float   xvmax;			// $3=0.000 (x v_max, mm/min)
    float   yvmax;			// $4=0.000 (y v_max, mm/min)
    float   zvmax;			// $5=0.000 (z v_max, mm/min)
    float   xacc;			// $6=0.000 (x accel, mm/sec^2)
    float   yacc;			// $7=0.000 (y accel, mm/sec^2)
    float   zacc;			// $8=0.000 (z accel, mm/sec^2)
    int     plen;			// $9=10 (step pulse, usec)
    float   feed;			// $10=250.000 (default feed, mm/min)
    bool    xstep;			// $11=192 (step port invert mask, int:11000000)
    bool    ystep;
    bool    zstep;
    bool    xdir;
    bool    ydir;
    bool    zdir;
    int     stepidle;		// $12=25 (step idle delay, msec)
    float   dev;			// $13=0.050 (junction deviation, mm)
    float   arctol;			// $14=0.000 (arc tolerance, mm)
    int     nkomma;			// $15=3 (n-decimals, int)
    bool    inch;			// $16=0 (report inches, bool)
    bool    autostart;		// $17=1 (auto start, bool)
    bool    invstep;		// $18=0 (invert step enable, bool)
    bool    hardlimit;		// $19=0 (hard limits, bool)
    bool    homecycle;		// $20=0 (homing cycle, bool)
    bool    xhome;			// $21=0 (homing dir invert mask, int:00000000)
    bool    yhome;
    bool    zhome;
    float   homefeed;		// $22=25.000 (homing feed, mm/min)
    float   homeseek;		// $23=250.000 (homing seek, mm/min)
    int     homedebounce;	// $24=100 (homing debounce, msec)
    float   homepulloff;	// $25=1.000 (homing pull-off, mm)
    } GRBLsettings;

typedef struct _GRBLpos
    {
    double  mposx,mposy,mposz;  // machine position
    double  wposx,wposy,wposz;  // work position
    } GRBLpos;

typedef struct _STEPCOOR
    {
    int		blockcnt;
    int		x,y,z;
    } STEPCOOR;

typedef struct _FCoor
    {
    float	x,y,z;
    int		blockcnt;
    } FCoor;

typedef enum _JOGDIR
    {
    LEFT,UPLEFT,UP,UPRIGHT,RIGHT,DOWNRIGHT,DOWN,DOWNLEFT,ZUP,ZDOWN
    } JOGDIR;

typedef std::vector<STEPCOOR>	STEPCOORvec;
typedef std::vector<FCoor>		FCoorvec;
typedef std::vector<DWORD>		DWordvec;

extern GRBLsettings     grblset;
extern GRBLpos          grblpos;
extern QString          grblstate;
extern FCoorvec 		gpoints;
extern DWordvec     	blocks;
extern int              curline,maxline;
extern double           zsecure;
extern bool             shift,moving,kill;

extern bool     ListPorts(QStringList& ports);
extern int      GRBLsim(QString& gcode, STEPCOORvec& ca, double reso);
extern int      FormatSettings(QStringList& ca);
extern float    readFloat(QString& qt);
extern int      readInt(QString& qt);
extern void     GetDimension(STEPCOORvec &ca, int& xsize, int& ysize, int& zsize);
extern void     CalcPoints(STEPCOORvec &ca);

#endif // GLOBALS_H
