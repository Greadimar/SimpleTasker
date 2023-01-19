#ifndef SIMPLEREPLYTASK_H
#define SIMPLEREPLYTASK_H
#include "simpletask.h"
namespace ST {
template <class ...TReply> class SimpleReplyTask: public SimpleTask, public QEnableSharedFromThis<SimpleReplyTask<TReply...>>
{

public:
    static QSharedPointer<SimpleReplyTask<TReply...>> create(const QString& name){
        //qDebug() << "crate replyt";
        return QSharedPointer<EnableMakeQShared<SimpleReplyTask<TReply...>>>::create(name);}
    //    template <typename ...TArgs> static QSharedPointer<SimpleReplyTask<TReply...>> createCustom(const QString& name, TArgs&&...){
    //        qDebug() << "defaultCustom";
    //    }
    virtual ~SimpleReplyTask() override{

    }

public:
    std::function<void(TReply&&...)> onSuccess;
    std::function<void()> onFail;


    bool run() override {
        mStartTime = std::chrono::system_clock::now();
        if (!runner){
            onLog(QStringLiteral("Task: ") + this->mName + QStringLiteral(" - can't be launched, nothing to execute"));
            mEndTime = mStartTime = std::chrono::system_clock::now();
            mTaskState = TaskState::fail;
            return false;
        }
        if (mHasDelay){
            return runWithDelay();
        }
        mTaskState = TaskState::waiting;
        runner();
        onLog(QStringLiteral("Task: ") + this->mName + QStringLiteral(" - launched"));
        if (!onSuccess && !onFail) mTaskState = TaskState::success;
        return true;
    };

    void makeRunner(QWeakPointer<SimpleReplyTask<TReply...>> wp, std::function<void()> customRunner){
        this->runner = [wp, customRunner](){if (!wp) return; customRunner();};
    }
    int getMaximumTries() const {return maximumTries;}
    void setMaximumTries(int value){ maximumTries = value;}
    int getTries() const {return tries;}


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
            shpRtr->onLog("Task: " + shpRtr->mName + " - executed");
            shpRtr->mTaskState = TaskState::success;
            if (shpRtr->onSuccess) shpRtr->onSuccess(std::forward<TReply>(var)...);
             shpRtr->mEndTime = std::chrono::system_clock::now();
        };
    }
    //get wrapper around success with logging and binded arguments
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
            shp->onLog(QStringLiteral("Task: ") + shp->mName + QString(" - no answer %1/%2").arg(shp->tries).arg(shp->getMaximumTries()));
            if (shp->tries < shp->maximumTries && shp->mTaskState == TaskState::waiting){
                shp->run();
                return ;
            }
            shp->mTaskState = TaskState::fail;
            if (shp->nextOnFail) shp->nextTask = shp->nextOnFail;
            if (shp->onFail) shp->onFail();
            shp->mEndTime = std::chrono::system_clock::now();
        };
    }
    //get wrapper around complex reply with logging
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
    bool runWithDelay(){
        if (!mTimerContext){
            onLog(QString("Task :%1 can't be launched, no timer context").arg(mName));
            return false;
        }
        mTaskState = TaskState::waiting;
        auto shared = QEnableSharedFromThis<SimpleReplyTask<TReply...>>::sharedFromThis();
        auto weak = shared.toWeakRef();
        QTimer::singleShot(mTimeToDelay, mTimerContext, [weak](){
            if (!weak) return;
            auto sh = weak.toStrongRef();
            if (sh->mTaskState == TaskState::waiting) sh->runner();
            else sh->onLog(QStringLiteral("Task: ") + sh->mName + QStringLiteral(" - forced to stop"));
        });
        onLog(QStringLiteral("Task: ") +
              this->mName +
              QStringLiteral(" - launched with delay %1 msec").arg(mTimeToDelay.count()));
        return true;
    }
    template <typename ...ReplyArgs>
    static void onReply(Shp<SimpleReplyTask<ReplyArgs...>>&& shp, bool success, TReply&&... data){    //TODO work on remove the copying
        if (success){
            auto&& onSuccess = shp->getOnSuccess();
            onSuccess(std::forward<TReply>(data)...);
            if (shp->mTaskState != TaskState::success){
                shp->tries++;
                if (shp->tries < shp->maximumTries){
                    shp->run();
                    return;
                }
                shp->mTaskState = TaskState::fail;
            }
        }
        else{
            auto&& onFail = shp->getOnFail();
            onFail();
        }
    }
};

}
#endif // SIMPLEREPLYTASK_H
