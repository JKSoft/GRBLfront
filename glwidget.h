#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include <QtOpenGL/QGLFunctions>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>

class GLWidget : public QGLWidget
    {
    Q_OBJECT

    GLuint      Prog;
    QString     Shader;
    QString     srcVertShader;
    GLuint      loadShader(const char* src, GLenum type);
    QPoint      lastpos;
    QMatrix4x4  pmat;
    float       xrota,yrota,zrota;
    float       xoff,yoff,size,zoom;

public:
    explicit GLWidget(QWidget *parent = 0);
    void        Init();
     ~GLWidget();

     QSize minimumSizeHint() const;
     QSize sizeHint() const;

signals:
    
public slots:
    
 protected:
     void initializeGL();
     void paintGL();
     void CheckProg(GLuint prog);
     void resizeGL(int width, int height);
     void mousePressEvent(QMouseEvent *event);
     void mouseMoveEvent(QMouseEvent *event);
     void wheelEvent(QWheelEvent *event);
    };

#endif // GLWIDGET_H
