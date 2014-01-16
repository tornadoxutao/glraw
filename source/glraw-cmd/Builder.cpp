
#include "Builder.h"

#include <iostream>

#include <QDebug>
#include <QCoreApplication>
#include <QCommandLineOption>

#include <glraw/MirrorEditor.h>
#include <glraw/ScaleEditor.h>
#include <glraw/FileWriter.h>
#include <glraw/Converter.h>
#include <glraw/CompressionConverter.h>


#include "CommandLineOption.h"
#include "Conversions.h"

namespace
{
    void messageHandler(QtMsgType type, const QMessageLogContext & context, const QString & message)
    {
        if (type == QtDebugMsg)
            return;

        std::cerr << message.toStdString() << std::endl;
    }
}

Builder::Builder()
:   m_converter(nullptr)
,   m_writer(new glraw::FileWriter())
,   m_manager(m_writer)
{
    initialize();
}

Builder::~Builder()
{
}

QList<CommandLineOption> Builder::commandLineOptions()
{
    QList<CommandLineOption> options;
    
    options.append({
        QStringList() << "h" << "help",
        "Displays this help.",
        QString(),
        &Builder::help
    });
    
    options.append({
        QStringList() << "q" << "quiet",
        "Suppresses any output",
        QString(),
        &Builder::quiet
    });
    
    options.append({
        QStringList() << "n" << "no-suffixes",
        "Disables file suffixes.",
        QString(),
        &Builder::noSuffixes
    });

    options.append({
        QStringList() << "f" << "format",
        "Output format (default: GL_RGBA)",
        "format",
        &Builder::format
    });

    options.append({
        QStringList() << "t" << "type",
        "Output type (default: GL_UNSIGNED_BYTE)",
        "type",
        &Builder::type
    });
    
    options.append({
        QStringList() << "compressed-format",
        "Output format (default: GL_COMPRESSED_RGBA)",
        "format",
        &Builder::compressedFormat
    });
    
    options.append({
        QStringList() << "r" << "raw",
        "Writes files without header.",
        QString(),
        &Builder::raw
    });

    options.append({
        QStringList() << "mv" << "mirror-vertical",
        "Mirrors the image vertically.",
        QString(),
        &Builder::mirrorVertical
    });

    options.append({
        QStringList() << "mh" << "mirror-horizontal", 
        "Mirrors the image horizontally.",
        QString(),
        &Builder::mirrorHorizontal
    });

    options.append({
        QStringList() << "s" << "scale",
        "Scales the image.",
        "decimal",
        &Builder::scale
    });

    options.append({
        QStringList() << "ws" << "width-scale",
        "Scales the width.",
        "decimal",
        &Builder::widthScale
    });

    options.append({
        QStringList() << "hs" << "height-scale",
        "Scales the height.",
        "decimal",
        &Builder::heightScale
    });

    options.append({
        QStringList() << "width",
        "Sets the width in px.",
        "integer",
        &Builder::width
    });

    options.append({
        QStringList() << "height",
        "Sets the height in px.",
        "integer",
        &Builder::height
    });

    options.append({
        QStringList() << "transform-mode",
        "Transformation mode used for resizing        "
        "(default: nearest)",
        "mode",
        &Builder::transformMode
    });
    
    options.append({
        QStringList() << "aspect-ratio-mode",
        "Aspect ratio mode used for resizing          "
        "(default: IgnoreAspectRatio)",
        "mode",
        &Builder::aspectRatioMode
    });
    
    options.append({
        QStringList() << "shader",
        "Applies a fragment shader before conversion  "
        "(see for example data/grayscale.frag)",
        "source",
        &Builder::shader
    });

    return options;
}

void Builder::initialize()
{
    m_parser.setApplicationDescription("Converts Qt supported images to an OpenGL compatible raw format.");
    m_parser.addVersionOption();

    m_parser.addPositionalArgument("sources", "Source files with Qt-supported image format.");

    for (auto option : commandLineOptions())
    {
        m_parser.addOption(QCommandLineOption(
            option.names,
            option.description,
            option.valueName
        ));
        
        for (auto name : option.names)
        {
            m_configureMethods.insert(
                name,
                option.configureMethod
            );
        }
    }
}

void Builder::process(const QCoreApplication & app)
{
    if (app.arguments().size() == 1)
    {
        showHelp();
        return;
    }
    
    m_parser.process(app);
    
    m_manager.setConverter(nullptr);
    m_converter = nullptr;
    
    for (auto option : m_parser.optionNames())
    {
        if (!(this->*m_configureMethods.value(option))(option))
            return;
    }

    if (m_converter == nullptr)
        m_converter = new glraw::Converter();
    
    m_manager.setConverter(m_converter);

    QStringList sources = m_parser.positionalArguments();
    
    if (sources.size() < 1)
    {
        qDebug() << "No source files passed in.";
        return;
    }
    
    for (auto source : sources)
        m_manager.process(source);
}

bool Builder::help(const QString & name)
{
    showHelp();
    return false;
}

bool Builder::quiet(const QString & name)
{
    qInstallMessageHandler(messageHandler);
    return true;
}

bool Builder::noSuffixes(const QString & name)
{
    m_writer->setSuffixesEnabled(false);
    return true;
}

bool Builder::format(const QString & name)
{
    QString formatString = m_parser.value(name);
    
    if (!Conversions::isFormat(formatString))
    {
        qDebug() << qPrintable(formatString) << "is not a format.";
        return false;
    }
    
    if (m_converter == nullptr)
        m_converter = new glraw::Converter();
    
    glraw::Converter * converter = dynamic_cast<glraw::Converter *>(m_converter);
    
    if (converter == nullptr)
    {
        qDebug() << "You can either specify a compressed format or an uncompressed format and type.";
        return false;
    }
    
    converter->setFormat(Conversions::stringToFormat(formatString));

    return true;
}

bool Builder::type(const QString & name)
{
    QString formatString = m_parser.value(name);
    
    if (!Conversions::isType(formatString))
    {
        qDebug() << qPrintable(formatString) << "is not a type.";
        return false;
    }
    
    if (m_converter == nullptr)
        m_converter = new glraw::Converter();
    
    glraw::Converter * converter = dynamic_cast<glraw::Converter *>(m_converter);
    
    if (converter == nullptr)
    {
        qDebug() << "You can either specify a compressed format or an uncompressed format and type.";
        return false;
    }
    
    converter->setType(Conversions::stringToType(formatString));

    return true;
}

bool Builder::compressedFormat(const QString & name)
{
    QString formatString = m_parser.value(name);
    
    if (!Conversions::isCompressedFormat(formatString))
    {
        qDebug() << qPrintable(formatString) << "is not a compressed format.";
        return false;
    }
    
    if (m_converter == nullptr)
        m_converter = new glraw::CompressionConverter();
    
    glraw::CompressionConverter * converter = dynamic_cast<glraw::CompressionConverter *>(m_converter);
    
    if (converter == nullptr)
    {
        qDebug() << "You can either specify a compressed format or an uncompressed format and type.";
        return false;
    }
    
    converter->setCompressedFormat(Conversions::stringToCompressedFormat(formatString));
    
    return true;
}

bool Builder::raw(const QString & name)
{
    m_writer->setHeaderEnabled(false);
    return true;
}

bool Builder::mirrorVertical(const QString & name)
{
    const QString editorName = "MirrorEditor";
    if (!editorExists(editorName))
        appendEditor(editorName, new glraw::MirrorEditor());
    
    auto e = editor<glraw::MirrorEditor>(editorName);
    
    e->setVertical(true);
    
    return true;
}

bool Builder::mirrorHorizontal(const QString & name)
{
    const QString editorName = "MirrorEditor";
    if (!editorExists(editorName))
        appendEditor(editorName, new glraw::MirrorEditor());
    
    auto e = editor<glraw::MirrorEditor>(editorName);
    
    e->setHorizontal(true);
    
    return true;
}

bool Builder::scale(const QString & name)
{
    QString scaleString = m_parser.value(name);
    
    bool ok;
    float scale = scaleString.toFloat(&ok);
    if (!ok)
    {
        qDebug() << scaleString << "isn't a float.";
        return false;
    }

    const QString editorName = "ScaleEditor";
    if (!editorExists(editorName))
        appendEditor(editorName, new glraw::ScaleEditor());
    
    auto e = editor<glraw::ScaleEditor>(editorName);

    e->setScale(scale);
    
    return true;
}

bool Builder::widthScale(const QString & name)
{
    QString widthScaleString = m_parser.value(name);
    
    bool ok;
    float widthScale = widthScaleString.toFloat(&ok);
    if (!ok)
    {
        qDebug() << widthScaleString << "isn't a float.";
        return false;
    }

    const QString editorName = "ScaleEditor";
    if (!editorExists(editorName))
        appendEditor(editorName, new glraw::ScaleEditor());
    
    auto e = editor<glraw::ScaleEditor>(editorName);

    e->setWidthScale(widthScale);
    
    return true;
}

bool Builder::heightScale(const QString & name)
{
    QString heighScaleString = m_parser.value(name);
    
    bool ok;
    float heightScale = heighScaleString.toFloat(&ok);
    if (!ok)
    {
        qDebug() << heighScaleString << "isn't a float.";
        return false;
    }

    const QString editorName = "ScaleEditor";
    if (!editorExists(editorName))
        appendEditor(editorName, new glraw::ScaleEditor());
    
    auto e = editor<glraw::ScaleEditor>(editorName);

    e->setHeightScale(heightScale);
    
    return true;
}

bool Builder::width(const QString & name)
{
    QString widthString = m_parser.value(name);
    
    bool ok;
    int width = widthString.toInt(&ok);
    if (!ok)
    {
        qDebug() << widthString << "isn't a int.";
        return false;
    }

    const QString editorName = "ScaleEditor";
    if (!editorExists(editorName))
        appendEditor(editorName, new glraw::ScaleEditor());
    
    auto e = editor<glraw::ScaleEditor>(editorName);

    e->setWidth(width);
    
    return true;
}

bool Builder::height(const QString & name)
{
    QString heightString = m_parser.value(name);
    
    bool ok;
    int height = heightString.toInt(&ok);
    if (!ok)
    {
        qDebug() << heightString << "isn't a int.";
        return false;
    }

    const QString editorName = "ScaleEditor";
    if (!editorExists(editorName))
        appendEditor(editorName, new glraw::ScaleEditor());
    
    auto e = editor<glraw::ScaleEditor>(editorName);

    e->setHeight(height);
    
    return true;
}

bool Builder::transformMode(const QString & name)
{
    QString modeString = m_parser.value(name);
    
    if (!Conversions::isTransformationMode(modeString))
    {
        qDebug() << qPrintable(modeString) << "is not a transformation mode.";
        return false;
    }

    const QString editorName = "ScaleEditor";
    if (!editorExists(editorName))
        appendEditor(editorName, new glraw::ScaleEditor());
    
    auto e = editor<glraw::ScaleEditor>(editorName);
    
    e->setTransformationMode(
        Conversions::stringToTransformationMode(modeString)
    );

    return true;
}

bool Builder::aspectRatioMode(const QString & name)
{
    QString modeString = m_parser.value(name);
    
    if (!Conversions::isAspectRatioMode(modeString))
    {
        qDebug() << qPrintable(modeString) << "is not a transformation mode.";
        return false;
    }

    const QString editorName = "ScaleEditor";
    if (!editorExists(editorName))
        appendEditor(editorName, new glraw::ScaleEditor());
    
    auto e = editor<glraw::ScaleEditor>(editorName);
    
    e->setAspectRatioMode(
        Conversions::stringToAspectRatioMode(modeString)
    );

    return true;
}

bool Builder::shader(const QString & name)
{
    QString sourcePath = m_parser.value(name);
    
    if (!m_converter->setFragmentShader(sourcePath))
        return false;
    
    return true;
}

bool Builder::editorExists(const QString & key)
{
    return m_editors.contains(key);
}

void Builder::appendEditor(const QString & key, glraw::ImageEditorInterface * editor)
{
    m_manager.appendImageEditor(editor);
    m_editors.insert(key, editor);
}

void Builder::showHelp() const
{
   qDebug() << qPrintable(m_parser.helpText()) << R"(
Formats:          Types:                     Transformation Modes:
  GL_RED            GL_UNSIGNED_BYTE           nearest
  GL_BLUE           GL_BYTE                    linear
  GL_GREEN          GL_UNSIGNED_SHORT        
  GL_RG             GL_SHORT                 Aspect Ratio Modes:
  GL_RGB            GL_UNSIGNED_INT            IgnoreAspectRatio
  GL_BGR            GL_INT                     KeepAspectRatio
  GL_RGBA           GL_FLOAT                   KeepAspectRatioByExpanding
  GL_BGRA

Compressed Formats:
  GL_COMPRESSED_RED
  GL_COMPRESSED_RG
  GL_COMPRESSED_RGB
  GL_COMPRESSED_RGBA)";
#ifdef GL_ARB_texture_compression_rgtc
    qDebug() <<
R"(  GL_COMPRESSED_RED_RGTC1
  GL_COMPRESSED_SIGNED_RED_RGTC1
  GL_COMPRESSED_RG_RGTC2
  GL_COMPRESSED_SIGNED_RG_RGTC2)";
#endif
#ifdef GL_ARB_texture_compression_bptc
    qDebug() <<
R"(  GL_COMPRESSED_RGBA_BPTC_UNORM
  GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
  GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT)";
#endif
#ifdef GL_EXT_texture_compression_s3tc
    qDebug() <<
R"(  GL_COMPRESSED_RGB_S3TC_DXT1_EXT
  GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
  GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
  GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)";
#endif
}
