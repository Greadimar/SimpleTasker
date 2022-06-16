#ifndef SIMPLETASKS_H
#define SIMPLETASKS_H
#include "functional"
#include <QtGlobal>
#include <QSharedPointer>
#include <QEnableSharedFromThis>
#include <chrono>
#include <QTimer>
#include <QDebug>
#include <QTime>
#include <QThread>
#include "tasktools.h"
namespace ST {
constexpr bool dbgTasks{false};
struct CounterTasks{
    static int counter;
};
enum class TaskImportance{
    common,
    critical
};

enum class TaskState
{
    idle,
    waiting,
    success,
    fail
};
template <class T> using Shp = QSharedPointer<T>;

using timepoint = std::chrono::time_point<std::chrono::system_clock>;
struct TaskLog{
    enum TaskLogStatus{
        common,
        error
    };
    TaskLog(const QString& msg, const TaskLog::TaskLogStatus& logstatus = common):
        msg(msg), status(logstatus){}

    QString msg;
    TaskLogStatus status{common};

};

class SimpleTask
{
protected:    //for creating purposes
    template <class TBase>
    struct EnableMakeQShared : public TBase {
        template <typename ...Arg> EnableMakeQShared(Arg&&...arg) :TBase(std::forward<Arg>(arg)...) {}    //we use it to make sure that TBase is available for create
    };
public:
    static QSharedPointer<SimpleTask> create(const QString& name = "unknown task"){
        return QSharedPointer<EnableMakeQShared<SimpleTask>>::create(name);
    }
    virtual ~SimpleTask();
    std::function<void()> runner;
    virtual bool run();
    QSharedPointer<SimpleTask> nextTask;
    QSharedPointer<SimpleTask> nextOnFail;
    std::function<void(const QString&)> logEmitter;
    QString getName() const;
    void setName(const QString &value);

    TaskState getState() const;
    void setState(const TaskState &value);

    TaskImportance getImportance() const;
    void setImportance(const TaskImportance &value);

    QList<TaskLog> getServiceMsgs() const;

    void delayRun(QObject* timerContext, std::chrono::milliseconds delay){
        mHasDelay = true;
        mTimerContext = timerContext;
        mTimeToDelay = delay;
    }
    bool isDelayedRun() const {return mHasDelay;}
    std::chrono::milliseconds getTimeToDelay() const {return mTimeToDelay;}
    virtual bool checkForReady(){
        return taskState != TaskState::waiting;
    }
protected:
    explicit SimpleTask(const QString& name = "unknown task");
    QList<TaskLog> serviceMsgs;
    QString name;
    TaskState taskState{TaskState::idle};
    TaskImportance mImportance{TaskImportance::common};
    using timepoint = TaskTools::timepoint;
    timepoint mStartTime;
    timepoint mEndTime;

    bool mHasDelay{false};
    QObject* mTimerContext{nullptr};
    std::chrono::milliseconds mTimeToDelay{0};

    void onLog(const QString& msg, const TaskLog::TaskLogStatus& status = TaskLog::common){
        if constexpr (dbgTasks) qDebug() << "ruining" << name;
        serviceMsgs << TaskLog{msg, status};
        if (logEmitter) logEmitter(msg);
    }
};


//template <typename Head, typename ...Tail> struct var_alias{
////    static_assert ((std::is_same_v<Head, Tail> && ...), "Types must be same");
//    using type = Head;
//};
//template <typename ...Args>
//using var_ali

using ShpSimpleTask = QSharedPointer<SimpleTask>;
using WkpSimpleTask = QWeakPointer<SimpleTask>;

}

#endif // SIMPLETASKS_H
