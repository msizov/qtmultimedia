/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "directshowplayercontrol.h"

#include "directshowplayerservice.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmath.h>

static int volumeToDecibels(int volume)
{
    if (volume == 0) {
        return -10000;
    } else if (volume == 100) {
        return 0;
#ifdef QT_USE_MATH_H_FLOATS
    } else if (sizeof(qreal) == sizeof(float)) {
        return qRound(::log10f(float(volume) / 100) * 5000);
#endif
    } else {
        return qRound(::log10(qreal(volume) / 100) * 5000);
    }
}

static int decibelsToVolume(int dB)
{
    if (dB == -10000) {
        return 0;
    } else if (dB == 0) {
        return 100;
    } else {
        return qRound(100 * qPow(10, qreal(dB) / 5000));
    }
}

DirectShowPlayerControl::DirectShowPlayerControl(DirectShowPlayerService *service, QObject *parent)
    : QMediaPlayerControl(parent)
    , m_service(service)
    , m_audio(0)
    , m_updateProperties(0)
    , m_state(QMediaPlayer::StoppedState)
    , m_status(QMediaPlayer::NoMedia)
    , m_error(QMediaPlayer::NoError)
    , m_streamTypes(0)
    , m_volume(100)
    , m_muted(false)
    , m_position(0)
    , m_pendingPosition(-1)
    , m_duration(0)
    , m_playbackRate(0)
    , m_seekable(false)
{
}

DirectShowPlayerControl::~DirectShowPlayerControl()
{
    if (m_audio)
        m_audio->Release();
}

QMediaPlayer::State DirectShowPlayerControl::state() const
{
    return m_state;
}

QMediaPlayer::MediaStatus DirectShowPlayerControl::mediaStatus() const
{
    return m_status;
}

qint64 DirectShowPlayerControl::duration() const
{
    return m_duration;
}

qint64 DirectShowPlayerControl::position() const
{
    if (m_pendingPosition != -1)
        return m_pendingPosition;

    return const_cast<qint64 &>(m_position) = m_service->position();
}

void DirectShowPlayerControl::setPosition(qint64 position)
{
    if (m_status == QMediaPlayer::EndOfMedia) {
        m_status = QMediaPlayer::LoadedMedia;
        emit mediaStatusChanged(m_status);
    }

    if (m_state == QMediaPlayer::StoppedState && m_pendingPosition != position) {
        m_pendingPosition = position;
        emit positionChanged(m_pendingPosition);
        return;
    }

    m_service->seek(position);
    m_pendingPosition = -1;
}

int DirectShowPlayerControl::volume() const
{
    return m_volume;
}

void DirectShowPlayerControl::setVolume(int volume)
{
    int boundedVolume = qBound(0, volume, 100);

    if (m_volume == boundedVolume)
        return;

    m_volume = boundedVolume;

    if (!m_muted)
        setVolumeHelper(m_volume);

    emit volumeChanged(m_volume);
}

bool DirectShowPlayerControl::isMuted() const
{
    return m_muted;
}

void DirectShowPlayerControl::setMuted(bool muted)
{
    if (m_muted == muted)
        return;

    m_muted = muted;

    setVolumeHelper(m_muted ? 0 : m_volume);

    emit mutedChanged(m_muted);
}

void DirectShowPlayerControl::setVolumeHelper(int volume)
{
    if (!m_audio)
        return;

    m_audio->put_Volume(volumeToDecibels(volume));
}

int DirectShowPlayerControl::bufferStatus() const
{
    return m_service->bufferStatus();
}

bool DirectShowPlayerControl::isAudioAvailable() const
{
    return m_streamTypes & DirectShowPlayerService::AudioStream;
}

bool DirectShowPlayerControl::isVideoAvailable() const
{
    return m_streamTypes & DirectShowPlayerService::VideoStream;
}

bool DirectShowPlayerControl::isSeekable() const
{
    return m_seekable;
}

QMediaTimeRange DirectShowPlayerControl::availablePlaybackRanges() const
{
    return m_service->availablePlaybackRanges();
}

qreal DirectShowPlayerControl::playbackRate() const
{
    return m_playbackRate;
}

void DirectShowPlayerControl::setPlaybackRate(qreal rate)
{
    if (m_playbackRate != rate) {
        m_service->setRate(rate);

        emit playbackRateChanged(m_playbackRate = rate);
    }
}

QMediaContent DirectShowPlayerControl::media() const
{
    return m_media;
}

const QIODevice *DirectShowPlayerControl::mediaStream() const
{
    return m_stream;
}

void DirectShowPlayerControl::setMedia(const QMediaContent &media, QIODevice *stream)
{
    m_pendingPosition = -1;

    m_media = media;
    m_stream = stream;

    m_updateProperties &= PlaybackRateProperty;

    m_service->load(media, stream);

    emit mediaChanged(m_media);
    emitPropertyChanges();
}

void DirectShowPlayerControl::play()
{
    playOrPause(QMediaPlayer::PlayingState);
}

void DirectShowPlayerControl::pause()
{
    playOrPause(QMediaPlayer::PausedState);
}

void DirectShowPlayerControl::playOrPause(QMediaPlayer::State state)
{
    if (m_status == QMediaPlayer::NoMedia || state == QMediaPlayer::StoppedState)
        return;
    if (m_status == QMediaPlayer::InvalidMedia) {
        setMedia(m_media, m_stream);
        if (m_error != QMediaPlayer::NoError)
            return;
    }

    m_state = state;

    if (m_pendingPosition != -1)
        setPosition(m_pendingPosition);

    if (state == QMediaPlayer::PausedState)
        m_service->pause();
    else
        m_service->play();

    emit stateChanged(m_state);
}

void DirectShowPlayerControl::stop()
{
    m_service->stop();
    emit stateChanged(m_state = QMediaPlayer::StoppedState);
}

void DirectShowPlayerControl::customEvent(QEvent *event)
{
    if (event->type() == QEvent::Type(PropertiesChanged)) {
        emitPropertyChanges();

        event->accept();
    } else {
        QMediaPlayerControl::customEvent(event);
    }
}

void DirectShowPlayerControl::emitPropertyChanges()
{
    int properties = m_updateProperties;
    m_updateProperties = 0;

    if ((properties & ErrorProperty) && m_error != QMediaPlayer::NoError)
        emit error(m_error, m_errorString);

    if (properties & PlaybackRateProperty)
        emit playbackRateChanged(m_playbackRate);

    if (properties & StreamTypesProperty) {
        emit audioAvailableChanged(m_streamTypes & DirectShowPlayerService::AudioStream);
        emit videoAvailableChanged(m_streamTypes & DirectShowPlayerService::VideoStream);
    }

    if (properties & PositionProperty)
        emit positionChanged(m_position);

    if (properties & DurationProperty)
        emit durationChanged(m_duration);

    if (properties & SeekableProperty)
        emit seekableChanged(m_seekable);

    if (properties & StatusProperty)
        emit mediaStatusChanged(m_status);

    if (properties & StateProperty)
        emit stateChanged(m_state);
}

void DirectShowPlayerControl::scheduleUpdate(int properties)
{
    if (m_updateProperties == 0)
        QCoreApplication::postEvent(this, new QEvent(QEvent::Type(PropertiesChanged)));

    m_updateProperties |= properties;
}

void DirectShowPlayerControl::updateState(QMediaPlayer::State state)
{
    if (m_state != state) {
        m_state = state;

        scheduleUpdate(StateProperty);
    }
}

void DirectShowPlayerControl::updateStatus(QMediaPlayer::MediaStatus status)
{
    if (m_status != status) {
        m_status = status;

        scheduleUpdate(StatusProperty);
    }
}

void DirectShowPlayerControl::updateMediaInfo(qint64 duration, int streamTypes, bool seekable)
{
    int properties = 0;

    if (m_duration != duration) {
        m_duration = duration;

        properties |= DurationProperty;
    }
    if (m_streamTypes != streamTypes) {
        m_streamTypes = streamTypes;

        properties |= StreamTypesProperty;
    }

    if (m_seekable != seekable) {
        m_seekable = seekable;

        properties |= SeekableProperty;
    }

    if (properties != 0)
        scheduleUpdate(properties);
}

void DirectShowPlayerControl::updatePlaybackRate(qreal rate)
{
    if (m_playbackRate != rate) {
        m_playbackRate = rate;

        scheduleUpdate(PlaybackRateProperty);
    }
}

void DirectShowPlayerControl::updateAudioOutput(IBaseFilter *filter)
{
    if (m_audio)
        m_audio->Release();

    m_audio = com_cast<IBasicAudio>(filter, IID_IBasicAudio);
    setVolumeHelper(m_muted ? 0 : m_volume);
}

void DirectShowPlayerControl::updateError(QMediaPlayer::Error error, const QString &errorString)
{
    m_error = error;
    m_errorString = errorString;

    if (m_error != QMediaPlayer::NoError)
        scheduleUpdate(ErrorProperty);
}

void DirectShowPlayerControl::updatePosition(qint64 position)
{
    if (m_position != position) {
        m_position = position;

        scheduleUpdate(PositionProperty);
    }
}
