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
constexpr bool dbgTasks{false};
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

namespace TaskTools{
template <typename Base>
inline QSharedPointer<Base>
shared_from_base(QEnableSharedFromThis<Base>* base)
{
    return base->sharedFromThis();
}
template <typename Base>
inline QSharedPointer<const Base>
shared_from_base(QEnableSharedFromThis<Base> const* base)
{
    return base->sharedFromThis();
}
template <typename That>
inline QSharedPointer<That>
shared_from(That* that)
{
    auto&& shb = shared_from_base(that);
    return shb.template staticCast<That>();
}

}

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
        template <typename ...Arg> EnableMakeQShared(Arg&&...arg) :TBase(std::forward<Arg>(arg)...) {}    //we use it to make sure that TBase

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


//template <typename Head, typename ...Tail> struct var_alias{
////    static_assert ((std::is_same_v<Head, Tail> && ...), "Types must be same");
//    using type = Head;
//};
//template <typename ...Args>
//using var_ali



template <class ...TReply> class SimpleReplyTask: public SimpleTask, public QEnableSharedFromThis<SimpleReplyTask<TReply...>>
{

public:
    static auto shp(QString name){
        return QSharedPointer<SimpleReplyTask<TReply...>>::create(name);
    }
    static QSharedPointer<SimpleReplyTask<TReply...>> create(const QString& name){
        return QSharedPointer<EnableMakeQShared<SimpleReplyTask<TReply...>>>::create(name);}
    //    template <typename ...TArgs> static QSharedPointer<SimpleReplyTask<TReply...>> createCustom(const QString& name, TArgs&&...){
    //        qDebug() << "defaultCustom";
    //    }
    bool run(){
        startTime = std::chrono::system_clock::now();
        if (runner){
            taskState = TaskState::waiting;
            onLog(QString("Задача: ") + this->name + QString("-  запущена"));
            runner();
            if constexpr (dbgTasks) qDebug() << Q_FUNC_INFO << "return true";
            if (!onSuccess && !onFail) taskState = TaskState::success;
            return true;
        }
        else{
            onLog(QString("Задача: ") + this->name + QString("- не может быть запущена, нечего исполнять"));
            endTime = startTime = std::chrono::system_clock::now();
            if constexpr (dbgTasks) qDebug() << Q_FUNC_INFO << "return false";
            if (!onSuccess && !onFail) taskState = TaskState::success;
            return false;
        }
        return false;
    }
    void delay(long delay){timeToDelay = delay;}
    std::function<void(TReply&&...)> onSuccess;
    std::function<void()> onFail;

    auto getOnReply(){
        auto shared = QEnableSharedFromThis<SimpleReplyTask<TReply...>>::sharedFromThis();
        return [sh = std::move(shared)](auto ...var) mutable{
            onReply(std::move(sh), var...);
        };
    }
    template <typename ...BindingArgs>
    auto getOnReplyWithBindedArgs(BindingArgs&&... bindingArgs){
        static_assert (sizeof ...(BindingArgs) != 0, "you have no arguments to bind, please use getOnReply instead");
        auto shared = QEnableSharedFromThis<SimpleReplyTask<TReply...>>::sharedFromThis();
        /* c++20 feature
         * template <typename ... Args>
            auto f(Args&& ... args){
                return [... args = std::forward<Args>(args)]{
                    // use args
                };
            }
            // c++17 could be done with following approach: https://stackoverflow.com/questions/47496358/c-lambdas-how-to-capture-variadic-parameter-pack-from-the-upper-scope
            // but it's a mess to care two parameters pack, so leaving like this for a while, anyway we bind only one bool
        */

        return [bindingArgs..., sh = std::move(shared)](auto ...var) mutable{
                onReply(std::move(sh), std::forward<BindingArgs>(bindingArgs)..., var...);
         };
    }
    template <typename ...ReplyArgs>
    static void onReply(Shp<SimpleReplyTask<ReplyArgs...>>&& shp, bool success, TReply... data){
        if (success){
            shp->onLog(QString("Задача: ") + shp->name + QString(" - выполнено"));
            shp->taskState = TaskState::success;
            if (shp->onSuccess) shp->onSuccess(std::forward<TReply>(data)...);
        }
        else{
            shp->tries++;
            shp->onLog(QString("Задача: ") + shp->name + QString(" - нет ответа"));
            shp->taskState = TaskState::fail;
            if (shp->tries < shp->maximumTries){
                shp->run();
                return ;
            }
            if (shp->onFail) shp->onFail();
        }
        shp->endTime = std::chrono::system_clock::now();
    }

    int getMaximumTries() const {return maximumTries;}
    void setMaximumTries(int value){ maximumTries = value;}

    int getTries() const {return tries;}
    public:
    explicit SimpleReplyTask(const QString& name): SimpleTask(name){
        m_needsReply = true;
    }
    private:
    int maximumTries{1};
    int tries{0};
    long timeToDelay;
};




class PauseTask: public SimpleReplyTask<>{
public:
    static QSharedPointer<PauseTask> create(const std::chrono::milliseconds& pausetime, QObject& contextParentForTimer,
                                            const QString& name = "pause"){
        return QSharedPointer<EnableMakeQShared<PauseTask>>::create(pausetime, contextParentForTimer, name);
    }
    bool run() override{
        auto shared = TaskTools::shared_from(this);
        auto wk = shared.toWeakRef();
        runner = [wk]() mutable{
            if (!wk) return;
                PauseTask::pauseRunner(wk.toStrongRef());
        };
        return SimpleReplyTask<>::run();
    }

protected:
    explicit PauseTask(const std::chrono::milliseconds& pausetime, QObject &contextForTimer,
                       const QString& name = "pause");
    ~PauseTask(){
    }
private:
    std::chrono::milliseconds pausetime;
    QObject& context;
    friend class SimpleTasker;
    static bool pauseRunner(Shp<PauseTask>&& sh){
        auto wk = sh.toWeakRef();
        QTimer::singleShot(sh->pausetime, &sh->context, sh->getOnReplyWithBindedArgs(true));
        return true;
    }
};
using ShpSimpleTask = QSharedPointer<SimpleTask>;
using WkpSimpleTask = QWeakPointer<SimpleTask>;



#endif // SIMPLETASKS_H
