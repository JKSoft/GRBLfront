#define GL_GLEXT_PROTOTYPES
#include "glwidget.h"
#include <QtOpenGL/QGLShaderProgram>
#include <QMessageBox>
#include <QMouseEvent>
#include <QWheelEvent>
#include "Globals.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

GLWidget::GLWidget(QWidget *parent) :
    QGLWidget(parent)
    {
    Init();
    }

void GLWidget::Init()
    {
    xrota = yrota = zrota = 0.; xoff = yoff = 0; zoom = 1.;
    }

GLWidget::~GLWidget()
    {
    glDeleteProgram(Prog);
    }

QSize GLWidget::minimumSizeHint() const
    {
    return QSize(50, 50);
    }

QSize GLWidget::sizeHint() const
    {
    return QSize(2000,2000);
    }

void GLWidget::initializeGL()
    {
    GLuint  iShader;
    GLuint  vertexShader;
    QColor  back(0,0,0);

    Shader = "\
            precision mediump float;\
            uniform vec4 color;\
            void main (void)\
            {\
                    gl_FragColor = color;\
            }";

    srcVertShader = "\
            attribute highp vec4 myVertex;\
            uniform mat4 mvpMatrix;\
            void main(void)\
            {\
                    gl_Position = mvpMatrix * myVertex;\
            }";

    qglClearColor(back);
    Prog = glCreateProgram();
    iShader = loadShader(qPrintable(Shader),GL_FRAGMENT_SHADER);
    vertexShader = loadShader(qPrintable(srcVertShader),GL_VERTEX_SHADER);
    glAttachShader(Prog,vertexShader);
    glAttachShader(Prog,iShader);
    glBindAttribLocation(Prog,0,"myVertex");
    glLinkProgram(Prog); CheckProg(Prog);
    }

void GLWidget::paintGL()
    {
    int             i,s1=0,s2=0,l=gpoints.size();
    int             mat_h,bl=blocks.size()-1;
    GLfloat         green[4]={0.,1.,0.,0.};
    GLfloat         yellow[4]={1.,.5,0.,0.};
    GLfloat         white[4]={1.,1.,1.,0.};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if(0 == l) return;
    for(i=0; i<l; i++)
        {
        if(curline < bl)
            {
            if(gpoints[i].blockcnt < blocks[curline]) s1 = i;
            if(gpoints[i].blockcnt < blocks[curline+1]) s2 = i;
            else break;
            }
        }
    QMatrix4x4 matrix;
    QMatrix4x4 trans(pmat);
    matrix.rotate(xrota,QVector3D(0.,1.,0.));
    matrix.rotate(yrota,QVector3D(1.,0.,0.));
    matrix.rotate(zrota,QVector3D(0.,0.,1.));
    matrix.translate(xoff,yoff);
    trans.scale(zoom);
    matrix *= trans;
    glUseProgram(Prog);
    mat_h = glGetUniformLocation(Prog,"mvpMatrix");
    glUniformMatrix4fv(mat_h,1,false,matrix.constData());
    mat_h = glGetUniformLocation(Prog,"color");
    if(s1)
        {
        glUniform4fv(mat_h,1,green);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(FCoor),gpoints.data());
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_LINE_STRIP,0,s1);
        }
    if(s2 > s1)
        {
        glUniform4fv(mat_h,1,yellow);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(FCoor),&gpoints.data()[s1]);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_LINE_STRIP,0,s2-s1);
        }
    if(s2 < l)
        {
        glUniform4fv(mat_h,1,white);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(FCoor),&gpoints.data()[s2]);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_LINE_STRIP,0,l-s2);
        }
    }

void GLWidget::resizeGL(int width, int height)
    {
    size = qMin(width, height);
    glViewport(0,0,width,height);
    pmat.setToIdentity();
    pmat.scale(1.,(float)width/(float)height);
    }

void GLWidget::mousePressEvent(QMouseEvent *event)
     {
     lastpos = event->globalPos();
     }

void GLWidget::mouseMoveEvent(QMouseEvent *event)
     {
     if(event->buttons() & Qt::LeftButton)
        {
        xrota += (event->globalPos().x() - lastpos.x()) / 5.;
        yrota += (event->globalPos().y() - lastpos.y()) / 5.;
        }
     if(event->buttons() & Qt::MiddleButton)
        {
        zrota += (event->globalPos().x() - lastpos.x()) / 5.;
        }
     if(event->buttons() & Qt::RightButton)
        {
        xoff += (event->globalPos().x() - lastpos.x()) / size;
        yoff -= (event->globalPos().y() - lastpos.y()) / size;
        }
     lastpos = event->globalPos(); update();
     }

 void GLWidget::wheelEvent(QWheelEvent *event)
    {
    zoom += event->delta() / 1000.; update();
    zoom = qMin(qMax(zoom,.1f),10.f);
    event->accept();
    }

GLuint GLWidget::loadShader(const char* src, GLenum type)
    {
    GLuint shader;
    GLint compiled;

    if(!(shader = glCreateShader(type))) return 0;
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled)
        {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1)
            {
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            QMessageBox::warning(this,"qGLES2-1", QString("Unable to compile the shader %1").arg(infoLog));
            free(infoLog);
            }
        glDeleteShader(shader);
        return 0;
        }
    return shader;
    }

void GLWidget::CheckProg(GLuint prog)
    {
    GLint   linked;

    glGetProgramiv(prog,GL_LINK_STATUS,&linked);
    if(!linked)
        {
        GLint infoLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLen);
        if(infoLen > 1)
            {
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(prog, infoLen, NULL, infoLog);
            QMessageBox::warning(this,"qGLES2-1", QString("Link failed %1").arg(infoLog));
            free(infoLog);
            }
        glDeleteProgram(prog);
        }
    }
