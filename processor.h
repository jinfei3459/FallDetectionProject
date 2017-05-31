#ifndef PROCESSOR_H
#define PROCESSOR_H
#include <QMutex>
#include <QRunnable>
#include <QString>
#include <QWaitCondition>
#include <QFuture>
#include <QElapsedTimer>
#include <QImage>

#include <opencv2/opencv.hpp>

#include <atomic>
#include <queue>

#include <libcaer/events/polarity.h>
#include <libcaer/events/frame.h>

#include "camerahandlerdavis.h"
#include "eventbuffer.h"

#include "settings.h"

class Processor:  public QObject,
    public CameraHandlerDavis::IDVSEventReciever,
    public CameraHandlerDavis::IFrameReciever
{
    Q_OBJECT
public:
    Processor();

    void start();
    void stop();

    void newEvent(const sDVSEventDepacked & event);
    void newFrame(const caerFrameEvent & frame);

    void run();

    EventBuffer &getBuffer()
    {
        return m_eventBuffer;
    }

    QImage getImg()
    {
        QMutexLocker locker(&m_frameMutex);
        return m_currFrame;
    }

    typedef struct sObjectStats {
        QPointF center;
        QPointF velocity;
        std::vector<QPointF> velocityHistory;
        QPointF velocityNorm;
        QPointF std;
        QRectF roi;
        QRectF bbox;
        size_t evCnt;
        uint64_t lastROIUpdate, deltaTimeLastDataUpdate;
        uint32_t id;

        sObjectStats()
        {
            id = -1;
            evCnt = 0;
            lastROIUpdate = 0;
            deltaTimeLastDataUpdate = 0;
        }
    } sObjectStats;

    QVector<sObjectStats> getStats()
    {
        QMutexLocker locker(&m_statsMutex);
        return m_stats;
    }
    float getProcessingFPS()
    {
        QMutexLocker locker(&m_statsMutex);
        return m_currProcFPS;
    }
    float getFrameFPS()
    {
        QMutexLocker locker(&m_frameMutex);
        return m_currFrameFPS;
    }

signals:
    void updateUI(QString msg);

private:
    void processImage();
    void updateStatistics(uint32_t elapsedTimeUs);
    void updateObjectStats(sObjectStats &st, uint32_t elapsedTimeUs);
    inline bool isInROI(const sDVSEventDepacked& e, const QRectF &roi);

    std::vector<cv::Rect> detect();
    void tracking(const std::vector<cv::Rect> &bboxes);

private:
    std::atomic_bool m_isRunning;
    QFuture<void> m_future;

    QMutex m_waitMutex;
    QWaitCondition m_waitForData;
    EventBuffer m_eventBuffer;

    QMutex m_queueMutex;
    std::queue<sDVSEventDepacked> m_eventQueue;

    QMutex m_frameMutex;
    float m_currFrameFPS;
    QElapsedTimer m_frameTimer;
    QImage m_currFrame;
    bool m_newFrameAvailable;
    QFuture<void> m_futureAnyncPedestrianDetector;
    u_int32_t m_nextId;

    // Time in us
    int m_timewindow;
    int m_updateStatsInterval;
    QElapsedTimer m_updateStatsTimer;

    QMutex m_statsMutex;
    QVector<sObjectStats> m_stats;
    float m_currProcFPS;
};
#endif // PROCESSOR_H
