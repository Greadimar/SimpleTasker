#ifndef SIMPLETASKS_H
#define SIMPLETASKS_H
#include "functional"
#include <QtGlobal>
#include <QSharedPointer>
#include <chrono>
#include <QTimer>
#include <QDebug>
#include <QTime>
#include <QThread>
constexpr bool dbgTasks{true};
struct CounterTasks{
    static int counter;
};
enum class TaskPriority{
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
        template <typename ...Arg> EnableMakeQShared(Arg&&...arg) :TBase(std::forward<Arg>(arg)...) {}
    };
public:
    //    static QSharedPointer<SimpleTask> create(const QString& name = "unknown task"){
    //        return Shp<SimpleTask>::create(name);
    //    }
    static QSharedPointer<SimpleTask> create(const QString& name = "unknown task"){
        return QSharedPointer<EnableMakeQShared<SimpleTask>>::create(name);
    }
    virtual ~SimpleTask();
    std::function<void()> runner;
    virtual bool run();

    QString getName() const;
    void setName(const QString &value);

    TaskState getState() const;
    void setState(const TaskState &value);
    bool getNeedsReply() const;

    TaskPriority getPriority() const;
    void setPriority(const TaskPriority &value);

    QList<TaskLog> getServiceMsgs() const;

protected:
    explicit SimpleTask(const QString& name = "unknown task");
    bool m_needsReply{false};
    QList<TaskLog> serviceMsgs;
    QString name;
    TaskState taskState{TaskState::idle};
    TaskPriority priority{TaskPriority::common};
    timepoint startTime;
    timepoint endTime;
    void onLog(const QString& msg, const TaskLog::TaskLogStatus& status = TaskLog::common){serviceMsgs << TaskLog{msg, status};}

};

template <class ...TReply> class SimpleReplyTask: public SimpleTask, public QEnableSharedFromThis<SimpleReplyTask<TReply...>>
{

public:
    static QSharedPointer<SimpleReplyTask<TReply...>> create(const QString& name){
        return QSharedPointer<EnableMakeQShared<SimpleReplyTask<TReply...>>>::create(name);}
//    template <typename ...TArgs> static QSharedPointer<SimpleReplyTask<TReply...>> createCustom(const QString& name, TArgs&&...){
//        qDebug() << "defaultCustom";
//    }
    bool run(){
        startTime = std::chrono::system_clock::now();
        if (runner){
            //qDebug() << Q_FUNC_INFO << "runner" << runCounter++;
            taskState = TaskState::waiting;
            onLog(QString("Задача: ") + this->name + QString("-  запущена"));
            runner();
            if constexpr (dbgTasks) qDebug() << Q_FUNC_INFO << "return true";
            return true;
        }
        else{
            onLog(QString("Задача: ") + this->name + QString("- не может быть запущена, нечего исполнять"));
            endTime = startTime = std::chrono::system_clock::now();
            if constexpr (dbgTasks) qDebug() << Q_FUNC_INFO << "return false";
            return false;
        }
    }
    void delay(long delay){timeToDelay = delay;}
    std::function<void(TReply&&...)> onSuccess;
    std::function<void()> onFail;
    // std::function<void(bool, TReply...)> onReply;
    auto getOnReply(){ return [=](auto ...var){
            onReply(var...);
        };}
    void onReply(bool success, TReply...data){
        if (success){
            onLog(QString("Задача: ") + name + QString(" - выполнено"));
            taskState = TaskState::success;
            if (onSuccess) onSuccess(std::forward<TReply>(data)...);
        }
        else{
            tries++;
            onLog(QString("Задача: ") + name + QString(" - нет ответа"));
            taskState = TaskState::fail;
            if (tries < maximumTries){
                run();
                return ;
            }
            if (onFail) onFail();
        }
        endTime = std::chrono::system_clock::now();
    }
    int getMaximumTries() const {return maximumTries;}
    void setMaximumTries(int value){ maximumTries = value;}

    int getTries() const {return tries;}
protected:
    explicit SimpleReplyTask(const QString& name): SimpleTask(name){
        m_needsReply = true;
    }
private:
    int maximumTries{1};
    int tries{0};
    long timeToDelay{0};
};

class PauseTask: public SimpleReplyTask<>{
public:
    static QSharedPointer<PauseTask> create(const std::chrono::milliseconds& pausetime, const QObject& contextParentForTimer,
                                            const QString& name = "pause"){
        return QSharedPointer<EnableMakeQShared<PauseTask>>::create(pausetime, contextParentForTimer, name);
    }
    bool run() override{
        auto shp = this->sharedFromThis();
        auto wkp = shp.toWeakRef();
        runner = [wkp](){
            if (!wkp){ qDebug() << "can;t pause, wkp = null"; return;}
            auto strp = wkp.toStrongRef().dynamicCast<PauseTask>();
            QTimer::singleShot(strp->pausetime, &strp->context, [strp](){strp->onReply(true);});
        };
        return SimpleReplyTask<>::run();
    }
protected:
    explicit PauseTask(const std::chrono::milliseconds& pausetime, const QObject& contextForTimer,
                       const QString& name = "pause");
private:
    std::chrono::milliseconds pausetime;
    const QObject& context;
};
using ShpSimpleTask = QSharedPointer<SimpleTask>;
using WkpSimpleTask = QWeakPointer<SimpleTask>;



#endif // SIMPLETASKS_H
