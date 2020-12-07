#include "simpletasks.h"
#include <QDebug>
int CounterTasks::counter{0};
SimpleTask::SimpleTask(const QString &name): name(name)
{
   if constexpr (dbgTasks) qDebug() << Q_FUNC_INFO << ++CounterTasks::counter;

}

SimpleTask::~SimpleTask()
{
   if constexpr (dbgTasks) qDebug() << Q_FUNC_INFO <<  --CounterTasks::counter;

}

bool SimpleTask::run(){
    startTime = std::chrono::system_clock::now();
    if (runner){
        runner();
        onLog(QString("Задача: ") + this->name + QString(" - запущена"));
        return true;
    }
    else{
        onLog(QString("Задача: ") + this->name + QString(" - не может быть запущена, нечего исполнять"));
        endTime = startTime = std::chrono::system_clock::now();
        return false;
    }
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


bool SimpleTask::getNeedsReply() const
{
    return m_needsReply;
}

TaskPriority SimpleTask::getPriority() const
{
    return priority;
}

void SimpleTask::setPriority(const TaskPriority &value)
{
    priority = value;
}

QList<TaskLog> SimpleTask::getServiceMsgs() const
{
    return serviceMsgs;
}



PauseTask::PauseTask(const std::chrono::milliseconds &pausetime, const QObject &contextForTimer, const QString &name):
    SimpleReplyTask<>(name), pausetime(pausetime), context(contextForTimer)
{

}
