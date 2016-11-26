#include "dsimageencodercontrol.h"
QT_BEGIN_NAMESPACE

DSImageEncoderControl::DSImageEncoderControl(QObject *parent)
    : QImageEncoderControl(parent), d_ptr(new DSImageEncoderControlPrivate)
{
    //Q_D(const DSImageEncoderControl);
    d_ptr->session = reinterpret_cast<DSCameraSession*>(parent);
}

QStringList DSImageEncoderControl::supportedImageCodecs() const
{
    return QStringList() << QStringLiteral("jpeg");
}

QString DSImageEncoderControl::imageCodecDescription(const QString &codecName) const
{
    if (codecName == QStringLiteral("jpeg"))
        return tr("JPEG image");

    return QString();
}


QList<QSize> DSImageEncoderControl::supportedResolutions(const QImageEncoderSettings &settings, bool *continuous) const
{
    Q_UNUSED(settings);
    Q_D(const DSImageEncoderControl);

    if (continuous)
        *continuous = false;

    return d->session->supportedResolutions();
}

QImageEncoderSettings DSImageEncoderControl::imageSettings() const
{
    Q_D(const DSImageEncoderControl);
    return d->session->imageEncoderSettings();
}

void DSImageEncoderControl::setImageSettings(const QImageEncoderSettings &settings)
{
    Q_D(DSImageEncoderControl);
    if (d->session->imageEncoderSettings() == settings)
        return;

    QImageEncoderSettings s = settings;
    applySettings(s);
    d->session->setImageEncoderSettings(s);
}

void DSImageEncoderControl::applySettings(QImageEncoderSettings& settings)
{
    if (settings.codec().isEmpty())
        settings.setCodec(QStringLiteral("jpeg"));

    QSize requestResolution = settings.resolution();
    QList<QSize> resolutions = supportedResolutions(settings, nullptr);
    if (resolutions.isEmpty() || resolutions.contains(requestResolution))
        return;

    // Find closest resolution from the list
    const int pixelCount = requestResolution.width() * requestResolution.height();
    int minimumGap = std::numeric_limits<int>::max();
    for (const QSize &size : qAsConst(resolutions)) {
        int gap = qAbs(pixelCount - size.width() * size.height());
        if (gap < minimumGap) {
            minimumGap = gap;
            requestResolution = size;
            if (gap == 0)
                break;
        }
    }
    settings.setResolution(requestResolution);
}

QT_END_NAMESPACE
