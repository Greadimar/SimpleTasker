#ifndef SIMPLETASKER_H
#define SIMPLETASKER_H

#include <QMutex>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QDebug>
#include <QPointer>
#include "simpletask.h"
#include "simplereplytask.h"
#include "taskcluster.h"
#include "taskplanner.h"
#include "type_traits"
#include <QTimer>
namespace ST {
constexpr static bool dbgTasker{false};

template <class T> using Shp = QSharedPointer<T>;

class SimpleTasker: public QObject
{
    Q_OBJECT
public:
    enum class TaskerState{
        idle,
        working,
    };
    explicit SimpleTasker(QObject *parent = nullptr);
    ~SimpleTasker(){
    }

    void setFirstTask(const ShpSimpleTask& startTask){
         finishedTaskSet.clear();
         this->taskPassed = 0;
         this->allCount = TaskPlanner::calcAllCount(startTask);
         this->branchCount = TaskPlanner::calcBranchCount(startTask);
         this->mStartTask = startTask;}
    void add(const ShpSimpleTask& addTask){
        if (!mStartTask){
            setFirstTask(addTask);
            return;
        }
        auto last = TaskPlanner::getLast(mStartTask);
        last->nextTask = addTask;
    }
    void add(const QVector<ShpSimpleTask>& tasks){
        if (tasks.isEmpty()) return;
        TaskPlanner::bindCoherentBranch(tasks);
        if (!mStartTask){
            mStartTask = tasks.first();
            return;
        }
        auto last = TaskPlanner::getLast(mStartTask);
        last->nextTask = tasks.first();
    }
    ShpSimpleTask getStartTask() const{

        return mStartTask;}
    ShpSimpleTask getCurTask() {return mCurTask;}
    template <class T>
    SimpleTasker& operator << (T&& taskData){
        add(std::forward<T>(taskData));
        return *this;
    }

    void remove(Shp<SimpleTask> task);
    void clean(){setFirstTask(nullptr);};

    void run();
    void runWithCommonInterval(std::chrono::milliseconds interval);
    void toPause();
    void toContinue();
    void stop();
    TaskerState getTaskerState() const;
    int getBranchCount() const;

    int getAllCount() const;

    int getTaskPassed() const;

signals:
    void infoMsg(QString msg);
    void sendPrg(double prg);
    //void serviceMsg(QString msg);
    void finished();
    void canceled();
private:
    const std::chrono::milliseconds checkNextTimeout{5};
    TaskerState taskerState{TaskerState::idle};
    QPointer<QTimer> timer;
    QSharedPointer<QMetaObject::Connection> connectionTimerCheckNext;
    ShpSimpleTask mStartTask;
    ShpSimpleTask mCurTask;
    QSet<ShpSimpleTask> finishedTaskSet;
    int taskPassed{0};  //TODO for calculating progress
    int allCount{0};
    int branchCount{0};
    //QVector<CoherentTask> coherentTaskList;
    std::chrono::milliseconds commonInterval{0};
    std::chrono::time_point<std::chrono::system_clock> timePointLastCheck;
    double lastPrgSent{0};
    template <bool interval = false> void toRun();
    template <bool interval = false> void runCoherentTask(ShpSimpleTask &task);
    template <bool interval = false> void checkForNextTask();

    bool checkTaskLooped() const{
        return finishedTaskSet.contains(mCurTask);
    }
    void toFinish();
};
}
#endif // SimpleTASKER_H
