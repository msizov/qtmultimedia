#ifndef DSIMAGEENCODERCONTROL_H
#define DSIMAGEENCODERCONTROL_H

#include <qimageencodercontrol.h>
#include "dscamerasession.h"

QT_BEGIN_NAMESPACE
//inspired by winrt implementation
class DSImageEncoderControlPrivate
{
public:
    ~DSImageEncoderControlPrivate(){}
    DSCameraSession* session;
};
class DSImageEncoderControl : public QImageEncoderControl
{
    Q_OBJECT
public:
    explicit DSImageEncoderControl(QObject *parent = 0);
    QStringList supportedImageCodecs() const Q_DECL_OVERRIDE;
    QString imageCodecDescription(const QString &codecName) const Q_DECL_OVERRIDE;

    QList<QSize> supportedResolutions(const QImageEncoderSettings &settings, bool *continuous) const Q_DECL_OVERRIDE;
    QImageEncoderSettings imageSettings() const Q_DECL_OVERRIDE;
    void setImageSettings(const QImageEncoderSettings &settings) Q_DECL_OVERRIDE;
private:
    void applySettings(QImageEncoderSettings& settings);
    QScopedPointer<DSImageEncoderControlPrivate> d_ptr;
    Q_DECLARE_PRIVATE(DSImageEncoderControl)
};

QT_END_NAMESPACE

#endif // DSIMAGEENCODERCONTROL_H
