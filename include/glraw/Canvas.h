
#pragma once

#include <glraw/glraw.h>

#include <QWindow>
#include <QOpenGLFunctions_3_2_Core>

class QImage;
class QByteArray;

namespace glraw
{

class GLRAW_API Canvas : public QWindow
{
public:
    Canvas();
    ~Canvas();

    void initializeGL();
    
    void loadTextureFromImage(QImage & image);
    QByteArray imageFromTexture(GLenum format, GLenum type);
    
    bool textureLoaded() const;

protected:
    static int byteSizeOf(GLenum type);
    static int numberOfElementsFor(GLenum format);
    
    
    QOpenGLContext m_context;
    GLuint m_texture;
    QOpenGLFunctions_3_2_Core gl;
    
};

} // namespace glraw
