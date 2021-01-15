#include "simpletasker.h"
#include <QDebug>
#include <QThread>
SimpleTasker::SimpleTasker(QObject* parent): QObject(parent)
{
}

void SimpleTasker::run()
{
    if constexpr(dbgTasker) qDebug() << Q_FUNC_INFO << "run" << QThread::currentThreadId() << coherentTaskList.count();
    if (!timer){
            qDebug() <<Q_FUNC_INFO <<"tim"<< QThread::currentThreadId();
        timer = new QTimer(this);
        timer->setTimerType(Qt::PreciseTimer);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, &SimpleTasker::checkForNextTask);
    }
    if (taskerState == TaskerState::working) return;
    QMutexLocker ml(&taskListMutex);
    if (curTaskIdx == coherentTaskList.size()){
        taskerState = TaskerState::idle;
        ml.unlock();
        emit finished();
    }
    else{
        taskerState = TaskerState::working;
        CoherentTask& task = coherentTaskList[curTaskIdx];
        runCoherentTask(task);
    }
}

void SimpleTasker::stop(){
    qDebug() <<Q_FUNC_INFO << QThread::currentThreadId();
    taskerState = TaskerState::idle;
    if (timer) timer->stop();
}


void SimpleTasker::runCoherentTask(CoherentTask& task)
{
   // if constexpr(dbgTasker) qDebug() << Q_FUNC_INFO <<"runcoh"<< QThread::currentThreadId();
    std::visit(VisitorRunner{},task);
    if (!timer->isActive()) timer->start(checkNextTimeout);
}

void SimpleTasker::checkForNextTask()
{
   // if constexpr(dbgTasker) qDebug() << Q_FUNC_INFO <<"checkFN"<< QThread::currentThreadId() << coherentTaskList.size();
    timer->stop();
    if (taskerState == TaskerState::idle || coherentTaskList.size() == 0){
        emit finished();
        return;
    }
    bool readyForNext{false};
    CoherentTask& task = coherentTaskList[curTaskIdx];
    std::visit([&readyForNext](auto&& arg){
        using T = std::decay_t<decltype (arg)>;
        if constexpr (std::is_same_v<T, ParallelCluster>){
            if constexpr(dbgTasker) qDebug() << "partask checkReady";
            readyForNext = arg.checkForReady();
        }
        else if constexpr (std::is_same_v<T, Shp<SimpleTask>>){
            if constexpr(dbgTasker) qDebug() << "cohtask checkReady";
            if (arg->getState() != TaskState::waiting) readyForNext = true;
        }
        else{
            qDebug()<<Q_FUNC_INFO << "wrong type";
        }
    }, task);

    if (readyForNext){
        curTaskIdx++;
        if (curTaskIdx == coherentTaskList.size()){
            taskerState = TaskerState::idle;
            emit finished();
        }
        else{
            CoherentTask& task = coherentTaskList[curTaskIdx];
            runCoherentTask(task);
        }
    }
    else{
            timer->start(checkNextTimeout);
    }
}
