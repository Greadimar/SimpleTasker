#include "taskcluster.h"
using namespace ST;
void SimulClusterTask::runSimultaneously(){
    auto curShp = startTask;
    if (!curShp) return;
    if (mInterval.count() == 0){
        while (curShp){
            curShp->run();
            curShp = curShp->nextTask;
        }
    }
}

void SimulClusterTask::runWithInterval(){
    auto shp = sharedFromThis();
    mTimer->connect(mTimer, &QTimer::timeout, mTimer, [weakCluster = shp.toWeakRef()](){
        if (!weakCluster){ return;} //timer stop?
        auto strCluster = weakCluster.toStrongRef();
        auto&& task = strCluster->curTask;
        if (task){
            task->run();
            strCluster->curTask = task->nextTask;
        }
        else{
            if (strCluster->checkForReady()) strCluster->setState(TaskState::success);
            strCluster->mTimer->stop();
            return;
        }

    });
    mTimer->start(mInterval);
}

void SimulClusterTask::setInterval(QObject *timerContext, std::chrono::milliseconds interval){
    mTimerContext = timerContext;
    mInterval = interval;
    if (!mTimer) mTimer = new QTimer();
    mTimer->setTimerType(Qt::PreciseTimer);
    mTimer->setParent(timerContext);
}

SimulClusterTask::~SimulClusterTask(){
    if (mTimer){
        if (!mTimer->parent()) delete mTimer;
    }
}

bool SimulClusterTask::run(){
    //
    mStartTime = std::chrono::system_clock::now();
    if (!startTask){
        onLog(QStringLiteral("TaskCluster: ") + this->mName + QStringLiteral(" - can't be launched, nothing to execute"));
        mEndTime = mStartTime = std::chrono::system_clock::now();
        return false;
    }
    if (mHasDelay){
        if (!mTimerContext){
            onLog(QStringLiteral("TaskCluster: '%1' can't be launched with delay, no context for timer").arg(this->mName));
            mTaskState = TaskState::fail;
            return false;
        }
        QTimer::singleShot(mTimeToDelay, Qt::PreciseTimer, mTimerContext, [=](){
            if (mInterval.count() == 0){
                runSimultaneously();
            }
            else{
                runWithInterval();
            }
        });
        onLog(QStringLiteral("TaskCluster: ") +
              this->mName +
              QStringLiteral(" - launched with delay %1 msec").arg(mTimeToDelay.count()));
    }
    else{
        if (mInterval.count() == 0){
            runSimultaneously();
        }
        else{
            runWithInterval();
        }
        onLog(QStringLiteral("TaskCluster: '") +
              this->mName +
              QStringLiteral("' - launched with delay %1 msec").arg(mTimeToDelay.count()));
    }
    mTaskState = TaskState::waiting;
    return true;
}

bool SimulClusterTask::checkForReady(){
    auto cur = startTask;
    bool isReady{cur->getState() == TaskState::success};
    while (cur){
        auto&& taskState = cur->getState();
        if (taskState == TaskState::waiting || taskState == TaskState::idle){
        isReady &= false;
        }
        cur = cur->nextTask;
    }
    if (!isReady &&
            ((std::chrono::system_clock::now() - mStartTime) > maxReadyWait)){
        setState(TaskState::fail);
    }
    else if(isReady){
        setState(TaskState::success);
    }
    return isReady;
}
