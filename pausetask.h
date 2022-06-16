#ifndef PAUSETASK_H
#define PAUSETASK_H
#include "simplereplytask.h"
namespace ST {

class PauseTask: public SimpleTask, public QEnableSharedFromThis<PauseTask>{
public:
    static QSharedPointer<PauseTask> create(const std::chrono::milliseconds& pausetime, QObject& contextParentForTimer,
                                            const QString& name = "pause"){
        return QSharedPointer<EnableMakeQShared<PauseTask>>::create(pausetime, contextParentForTimer, name);
    }
    std::function<void()> onPauseEnd;
    bool run() override{
        auto shared = TaskTools::shared_from(this);
        auto wk = shared.toWeakRef();
        runner = [wk]() mutable{
            if (!wk) return;
            PauseTask::pauseRunner(wk.toStrongRef());
        };
        bool virtRun = SimpleTask::run();
        setState(TaskState::waiting);
        return virtRun;
    }

protected:
    explicit PauseTask(const std::chrono::milliseconds& pausetime, QObject &contextForTimer,
                       const QString& name = "pause");
    virtual ~PauseTask() override{
    }
private:
    using SimpleTask::runner;
    std::chrono::milliseconds pausetime{9};
    QObject& context;
    friend class SimpleTasker;
    static bool pauseRunner(Shp<PauseTask>&& sh){
        auto wk = sh.toWeakRef();
        QTimer::singleShot(sh->pausetime, &sh->context, [wk]{
            if (wk) wk.toStrongRef()->onTimeout();
        });
        return true;
    }
    void onTimeout(){
        if (onPauseEnd) onPauseEnd();
        setState(TaskState::success);
    }
};
}
#endif // PAUSETASK_H
