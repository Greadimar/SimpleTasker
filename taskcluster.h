#ifndef TASKCLUSTER_H
#define TASKCLUSTER_H
#include "simpletask.h"
#include <QPointer>
namespace ST {
constexpr bool dbgCluster{false};

template <class T> using Shp = QSharedPointer<T>;
class SimulClusterTask: public SimpleTask, public QEnableSharedFromThis<SimulClusterTask>{
    template <class TBase>
    struct EnableMakeQShared : public TBase {
        template <typename ...Arg> EnableMakeQShared(Arg&&...arg) :TBase(std::forward<Arg>(arg)...) {}    //we use it to make sure that TBase
    };
private:
    SimulClusterTask(const ShpSimpleTask& firstInSimulBranch, const QString& name = "Кластер одновременных задач"): SimpleTask(name),
         startTask(firstInSimulBranch)
        {curTask = startTask;}
    SimulClusterTask(SimulClusterTask const&) = default;
    QPointer<QTimer> mTimer;
    std::chrono::milliseconds mInterval{0};
    std::chrono::seconds maxReadyWait{10};
    QObject* mIntervalTimerContext;

    void runSimultaneously();
    void runWithInterval();
    ShpSimpleTask curTask{nullptr};
public:
    static QSharedPointer<SimulClusterTask> create(const ShpSimpleTask& firstInSimulBranch,
                                                  const QString& name = "Кластер одновременных задач"){
        return QSharedPointer<EnableMakeQShared<SimulClusterTask>>::create(firstInSimulBranch, name);
    }
    void setInterval(QObject* timerContext, std::chrono::milliseconds interval);
    ~SimulClusterTask();
    bool run() override;
    friend bool operator==(const SimulClusterTask& lhs, const SimulClusterTask rhs);
private:
    bool checkForReady() override;
    ShpSimpleTask startTask{nullptr};

};
 inline bool operator==(const SimulClusterTask& lhs, const SimulClusterTask rhs){
     return ((lhs.getName() == rhs.getName()) && (lhs.startTask == rhs.startTask));
 }
using ShpSimulClusterTask = QSharedPointer<SimulClusterTask>;
}
#endif // TaskCluster_H
