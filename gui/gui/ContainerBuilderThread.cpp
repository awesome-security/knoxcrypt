#include "ContainerBuilderThread.h"

#include <QDebug>

#include <memory>

ContainerBuilderThread::ContainerBuilderThread(QObject *parent)
    : QThread(parent)
    , m_imageBuilder()
{
}

void ContainerBuilderThread::setSharedIO(teasafe::SharedCoreIO const &io)
{
    TeaLock lock(m_teaMutex);
    m_io = io;
    bool const sparseImage = true; // TODO: option to change this
    m_imageBuilder = std::make_shared<teasafe::MakeTeaSafe>(m_io, sparseImage);

    // register progress callback for imager
    std::function<void(teasafe::EventType)> fb(std::bind(&ContainerBuilderThread::imagerCallback, this, _1, m_io->blocks));
    m_imageBuilder->registerSignalHandler(fb);

}

SharedTeaSafe ContainerBuilderThread::getTeaSafe()
{
    TeaLock lock(m_teaMutex);
    return std::make_shared<teasafe::TeaSafe>(m_io);
}

void ContainerBuilderThread::run()
{
    this->buildTSImage();
}

void ContainerBuilderThread::buildTSImage()
{
    TeaLock lock(m_teaMutex);
    m_imageBuilder->buildImage();
}

void ContainerBuilderThread::imagerCallback(teasafe::EventType eventType, long const amount)
{
    static long val(0);
    if(eventType == teasafe::EventType::ImageBuildStart) {
        emit blockCountSignal(amount);
        emit setProgressLabelSignal("Building image...");
    }
    if(eventType == teasafe::EventType::ImageBuildUpdate) {
        emit blockWrittenSignal(++val);
    }
    if(eventType == teasafe::EventType::ImageBuildEnd) {
        val = 0;
        emit finishedBuildingSignal();
        emit closeProgressSignal();
    }
    if(eventType == teasafe::EventType::IVWriteEvent) {

    }
    if(eventType == teasafe::EventType::RoundsWriteEvent) {

    }
}
