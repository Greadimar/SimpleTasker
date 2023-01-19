#include "simpletasker.h"
#include <QDebug>
#include <cmath>
#include <QThread>
using namespace ST;
SimpleTasker::SimpleTasker(QObject* parent): QObject(parent)
{
}

void SimpleTasker::run()
{
    if (!mStartTask){
        emit infoMsg(QStringLiteral("Branch of task is empty. Job stopped"));
        emit finished();
        return;
    }
    toRun();
}

void SimpleTasker::runWithCommonInterval(std::chrono::milliseconds interval)
{
    if (!mStartTask){
        emit infoMsg(QStringLiteral("Branch of task is empty. Job stopped"));
        return;
    }
    this->commonInterval = interval;
    toRun<true>();
}

template<bool delay>
void SimpleTasker::toRun()
{
    lastPrgSent = 0;
    if (!timer){
        timer = new QTimer(this);
        timer->setTimerType(Qt::PreciseTimer);
        auto c = connect(timer, &QTimer::timeout, this, [=](){
            checkForNextTask<delay>();
        });
        connectionTimerCheckNext = QSharedPointer<QMetaObject::Connection>::create(c);
    }
    if (taskerState == TaskerState::working) return;
    taskerState = TaskerState::working;
    mCurTask = mStartTask;
    runCoherentTask<delay>(mCurTask);
    timer->start(checkNextTimeout);

}


void SimpleTasker::stop(){
    taskerState = TaskerState::idle;
    if (mCurTask) mCurTask->setState(TaskState::fail);
    if (timer) timer->stop();
    emit sendPrg(0);
    emit canceled();

}
void SimpleTasker::toPause()
{
    if (mCurTask) mCurTask->setState(TaskState::idle);
    if (timer) timer->stop();
}

void SimpleTasker::toContinue()
{
    if (mCurTask) mCurTask->setState(TaskState::idle);
    if (timer) timer->start(checkNextTimeout);
}

SimpleTasker::TaskerState SimpleTasker::getTaskerState() const
{
    return taskerState;
}

int SimpleTasker::getBranchCount() const
{
    return branchCount;
}

int SimpleTasker::getAllCount() const
{
    return allCount;
}

int SimpleTasker::getTaskPassed() const
{
    return taskPassed;
}

void SimpleTasker::toFinish(){
    taskerState = TaskerState::idle;
    if (timer) timer->stop();
    emit finished();

}



template <bool interval>
void SimpleTasker::runCoherentTask(ShpSimpleTask &task)
{
    if constexpr (interval){
        timePointLastCheck = std::chrono::system_clock::now();
    }
    if (!task){
        toFinish();
        return;
    }
    if (!task->logEmitter) task->logEmitter = [=](const QString& msg){
        emit infoMsg(msg);
    };
    taskPassed++;
    uint curPrg = std::ceil((taskPassed / static_cast<double>(branchCount)) * 100);
    if (curPrg > lastPrgSent) lastPrgSent = curPrg;
    if (lastPrgSent > 100) lastPrgSent = 100;
    emit sendPrg(curPrg);
    task->run();

    //if (!timer->isActive()) timer->start(checkNextTimeout);
}

template <bool interval>
void SimpleTasker::checkForNextTask()
{
    if constexpr (interval){
        auto now = std::chrono::system_clock::now();
        if (timePointLastCheck + std::chrono::milliseconds(commonInterval) > now){
            return;
        }
    }
    if (!mCurTask){
        qCritical() << "Current task in SimpleTasker is null. Aborting";
        taskerState = TaskerState::idle;
        return;
    }
    if (taskerState == TaskerState::idle){
        return;
    }
    bool readyForNext = mCurTask->checkForReady();
    if (!readyForNext) {
        return;
    }
    finishedTaskSet << mCurTask;
    if (!mCurTask->nextTask){
        toFinish();
        return;
    }
    mCurTask = mCurTask->nextTask;
    //double curPrg = 100 * (curTaskIdx/static_cast<double>(coherentTaskList.size()));

    runCoherentTask<interval>(mCurTask);
}

