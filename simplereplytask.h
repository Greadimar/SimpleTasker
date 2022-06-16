#ifndef SIMPLEREPLYTASK_H
#define SIMPLEREPLYTASK_H
#include "simpletask.h"
namespace ST {
template <class ...TReply> class SimpleReplyTask: public SimpleTask, public QEnableSharedFromThis<SimpleReplyTask<TReply...>>
{

public:
    static QSharedPointer<SimpleReplyTask<TReply...>> create(const QString& name){
        return QSharedPointer<EnableMakeQShared<SimpleReplyTask<TReply...>>>::create(name);}
    //    template <typename ...TArgs> static QSharedPointer<SimpleReplyTask<TReply...>> createCustom(const QString& name, TArgs&&...){
    //        qDebug() << "defaultCustom";
    //    }
    virtual ~SimpleReplyTask() override{}

public:
    std::function<void(TReply&&...)> onSuccess;
    std::function<void()> onFail;


    bool run() override {
        mStartTime = std::chrono::system_clock::now();
        if (!runner){
            onLog(QString("Task: ") + this->name + QString(" - can't be launched, nothing to execute"));
            mEndTime = mStartTime = std::chrono::system_clock::now();
            taskState = TaskState::fail;
            return false;
        }
        if (mHasDelay){
            if (!mTimerContext){
                onLog(QString("Task :%1 can't be launched with interval, no qobject context for timer"));
                return false;
            }
            taskState = TaskState::waiting;
            QTimer::singleShot(mTimeToDelay, mTimerContext, runner);
            onLog(QString("Task: ") +
                  this->name +
                  QString(" - launched with delay %1 msec").arg(mTimeToDelay.count()));
            return true;
        }
        taskState = TaskState::waiting;
        runner();
        onLog(QString("Task: ") + this->name + QString(" - launched"));
        if (!onSuccess && !onFail) taskState = TaskState::success;
        return true;
    };

    int getMaximumTries() const {return maximumTries;}
    void setMaximumTries(int value){ maximumTries = value;}
    int getTries() const {return tries;}

    //get wrapper around success with logging
    auto getOnReply() {
        auto shared = QEnableSharedFromThis<SimpleReplyTask<TReply...>>::sharedFromThis();
        auto weak = shared.toWeakRef();
        return [weak](auto ...var) mutable{
            if (!weak){
                qCritical() << "Current ReplyTask was deleted before recieving the answer";
                return;
            }
            onReply(std::move(weak.toStrongRef()), std::forward<decltype(var)>(var)...);
        };
    }
     //get wrapper around success with logging
    auto getOnSuccess(){
        auto shared = QEnableSharedFromThis<SimpleReplyTask<TReply...>>::sharedFromThis();
        auto weak = shared.toWeakRef();
        return [weak](auto ...var) mutable{
            if (!weak){
                qCritical() << "Current ReplyTask was deleted before recieving the answer";
                return;
            }
            auto shpRtr = weak.toStrongRef();
            shpRtr->onLog(QString("Task: ") + shpRtr->name + QString(" - complete"));
            shpRtr->taskState = TaskState::success;
            if (shpRtr->onSuccess) shpRtr->onSuccess(std::forward<TReply>(var)...);
             shpRtr->mEndTime = std::chrono::system_clock::now();
        };
    }
    template <typename ...BindingArgs>
    auto getOnSuccessWithBindedArgs(BindingArgs&&... bindingArgs) const{
        auto shared = QEnableSharedFromThis<SimpleReplyTask<TReply...>>::sharedFromThis();
        auto weak = shared.toWeakRef();
        return [bindingArgs..., weak](auto ...var) mutable{
            if (!weak){
                qCritical() << "Current ReplyTask was deleted before recieving the answer";
                return;
            }
            onReply(std::move(weak.toStrongRef()), true, var..., std::forward<BindingArgs>(bindingArgs)...);
        };
    }
    //get wrapper around fail with logging and rerun if needed
    auto getOnFail() {
        auto shared = QEnableSharedFromThis<SimpleReplyTask<TReply...>>::sharedFromThis();
        auto weak = shared.toWeakRef();
        return [weak]() mutable{
            if (!weak){
                qCritical() << "Current ReplyTask was deleted before recieving the answer";
                return;
            }
            auto shp = weak.toStrongRef();
            shp->tries++;
            shp->onLog(QString("Task: ") + shp->name + QString(" - no answer %1/%2").arg(shp->tries).arg(shp->getMaximumTries()));
            if (shp->tries < shp->maximumTries){
                shp->run();
                return ;
            }
            shp->taskState = TaskState::fail;
            if (shp->nextOnFail) shp->nextTask = shp->nextOnFail;
            if (shp->onFail) shp->onFail();
            shp->mEndTime = std::chrono::system_clock::now();
        };
    }
    template <typename ...BindingArgs>
    auto getOnReplyWithBindedArgs(BindingArgs&&... bindingArgs) {
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
        std::function<void(bool)> test = [bindingArgs..., sh = std::move(shared)](auto var) mutable{
                onReply(std::move(sh), var, std::forward<BindingArgs>(bindingArgs)...);
         };
         return test;
    }
protected:
explicit SimpleReplyTask(const QString& name): SimpleTask(name){
}
private:
    int maximumTries{1};
    int tries{0};

    template <typename ...ReplyArgs>
    static void onReply(Shp<SimpleReplyTask<ReplyArgs...>>&& shp, bool success, TReply&&... data){    //TODO work on remove the copying
        if (success){
            auto&& onSuccess = shp->getOnSuccess();
            onSuccess(std::forward<TReply...>(data...));
        }
        else{
            auto&& onFail = shp->getOnFail();
            onFail();
        }
    }

};

}
#endif // SIMPLEREPLYTASK_H
