#include "simpletask.h"
#include <QDebug>
using namespace ST;
int CounterTasks::counter{0};
SimpleTask::SimpleTask(const QString &name): mName(name)
{
    //DBG_FN (dbgTasks) << ++CounterTasks::counter << name;

}

SimpleTask::~SimpleTask()
{
    // DBG_FN (dbgTasks) <<  --CounterTasks::counter << name;
    while (nextTask) {
        nextTask = std::move(nextTask->nextTask);
    }
    while (nextOnFail) {
        nextOnFail = std::move(nextOnFail->nextOnFail);
    }
}

bool SimpleTask::run(){
    mStartTime = std::chrono::system_clock::now();
    if (!runner){
        onLog(QStringLiteral("Task: ") + this->mName + QStringLiteral(" - can't be launched, nothing to execute"));
        mEndTime = mStartTime = std::chrono::system_clock::now();
        return false;
    }
    if (mHasDelay){
        if (!mTimerContext){
            onLog(QStringLiteral("Task :%1 can't be launched with delay, no context for timer").arg(this->mName));
            return false;
        }
        QTimer::singleShot(mTimeToDelay, mTimerContext, runner);
        onLog(QStringLiteral("Task: ") +
              this->mName +
              QStringLiteral(" - launched with delay %1 msec").arg(mTimeToDelay.count()));
        return true;
    }
    runner();
    onLog(QStringLiteral("Task: ") + this->mName + QStringLiteral(" - launched"));
    mTaskState = TaskState::success;
    return true;
}

QString SimpleTask::getName() const
{
    return mName;
}

void SimpleTask::setName(const QString &value)
{
    mName = value;
}


TaskState SimpleTask::getState() const
{
    return mTaskState;
}

void SimpleTask::setState(const TaskState &value)
{
    mTaskState = value;
}


TaskImportance SimpleTask::getImportance() const
{
    return mImportance;
}

void SimpleTask::setImportance(const TaskImportance &value)
{
    mImportance = value;
}

QList<TaskLog> SimpleTask::getServiceMsgs() const
{
    return serviceMsgs;
}



