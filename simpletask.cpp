#include "simpletask.h"
#include <QDebug>
using namespace ST;
int CounterTasks::counter{0};
SimpleTask::SimpleTask(const QString &name): name(name)
{
   if constexpr (dbgTasks) qDebug() << Q_FUNC_INFO << ++CounterTasks::counter << name;

}

SimpleTask::~SimpleTask()
{
   if constexpr (dbgTasks) qDebug() << Q_FUNC_INFO <<  --CounterTasks::counter << name;

}

bool SimpleTask::run(){
    mStartTime = std::chrono::system_clock::now();
    if (!runner){
        onLog(QString("Task: ") + this->name + QString(" - can't be launched, nothing to execute"));
        mEndTime = mStartTime = std::chrono::system_clock::now();
        return false;
    }
    if (mHasDelay){
        if (!mTimerContext){
            onLog(QString("Task :%1 can't be launched with interval, no qobject context for timer").arg(this->name));
            return false;
        }
        QTimer::singleShot(mTimeToDelay, mTimerContext, runner);
        onLog(QString("Task: ") +
              this->name +
              QString(" - launched with delay %1 msec").arg(mTimeToDelay.count()));
        return true;
    }
    runner();
    onLog(QString("Task: ") + this->name + QString(" - launched"));
    taskState = TaskState::success;
    return true;
}

QString SimpleTask::getName() const
{
    return name;
}

void SimpleTask::setName(const QString &value)
{
    name = value;
}


TaskState SimpleTask::getState() const
{
    return taskState;
}

void SimpleTask::setState(const TaskState &value)
{
    taskState = value;
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



