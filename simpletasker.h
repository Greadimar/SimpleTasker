#ifndef SIMPLETASKER_H
#define SIMPLETASKER_H

#include <QMutex>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QDebug>
#include <QPointer>
#include "simpletasks.h"
#include "type_traits"
#include <variant>
#include <QTimer>
constexpr static bool dbgTasker{false};

template <class T> using Shp = QSharedPointer<T>;
class ParallelCluster{
public:
    ParallelCluster(const QList<Shp<SimpleTask>>& list, const QString& name):
        parallelTasks(list), name(name){}
    ParallelCluster(ParallelCluster const&) = default;
    ~ParallelCluster(){
        parallelTasks.clear();
    }
    QList<Shp<SimpleTask>> parallelTasks;
    QString name;
    TaskState state{TaskState::idle};
    bool checkForReady(){
        bool isReady{true};

        if constexpr (dbgTasker){
            int waitCount = 0;
            int sucCount = 0;
            int fCount = 0;
            for (auto&& t: parallelTasks){
                if (t->getState() == TaskState::waiting){
                    isReady = false;
                    waitCount++;
                }
                if (t->getState() == TaskState::success){
                    sucCount++;
                }
                if (t->getState() == TaskState::fail){
                    fCount++;
                }
            }
            qDebug() << Q_FUNC_INFO << QThread::currentThreadId() << isReady << "all w s f" <<parallelTasks.size()<< waitCount << sucCount << fCount;
        }
        else{
            for (auto&& t: parallelTasks){
                if (t->getState() == TaskState::waiting){
                    isReady = false;
                    break;
                }
            }
        }
        if (isReady) state = TaskState::success;
        return isReady;
    }
    timepoint startTime;
};
enum class TaskerState{
    idle,
    working,
};

using CoherentTask = std::variant<ParallelCluster, Shp<SimpleTask>>;

class SimpleTasker: public QObject
{
    Q_OBJECT
public:
    explicit SimpleTasker(QObject *parent = nullptr);
    ~SimpleTasker(){
        if constexpr(dbgTasker) qDebug() << Q_FUNC_INFO << coherentTaskList.size()<< CounterTasks::counter;
        clean();
        if constexpr(dbgTasker) qDebug()<< Q_FUNC_INFO << coherentTaskList.size() << CounterTasks::counter;

    }
    void addParallel(QList<Shp<SimpleTask>> list,
                     const QString& name = ""){
        QMutexLocker ml(&taskListMutex);
        coherentTaskList << ParallelCluster{{list}, name};
        coherentTasksCount++;
    }
    template <class TShpTask> void add(const Shp<TShpTask>& t){
        QMutexLocker ml(&taskListMutex);
        coherentTaskList << t.template dynamicCast<SimpleTask>();
        coherentTasksCount++;
    }
    void add(const QList<Shp<SimpleTask>>& list){
        QMutexLocker ml(&taskListMutex);
        for (auto& shpt: list){
            coherentTaskList.append(shpt);
        }
        coherentTasksCount += list.size();
    }
    void remove(Shp<SimpleTask> task){
        QMutexLocker ml(&taskListMutex);
        QMutableListIterator<CoherentTask> it(coherentTaskList);
        while(it.hasNext()){
            CoherentTask& t = it.next();
            std::visit([this, &task, &it](auto&& arg){ //visitor to deal with unique task or cluster
                using T = std::decay_t<decltype (arg)>;
                if constexpr (std::is_same_v<T, ParallelCluster>){ //todo : test if for compile time, still runtime polymorphysm
                    removeTaskFromCluster(task, arg);
                }
                else if constexpr (std::is_same_v<T, Shp<SimpleTask>>){
                    it.remove();
                    coherentTasksCount--;
                }
                else{

                }
            }, t);
        }
    }
    void clean(){
        QMutexLocker ml(&taskListMutex);
        coherentTaskList.clear();
        curTaskIdx = 0;
    }
    QVector<ShpSimpleTask> getAllTasksList(){
        QMutexLocker ml(&taskListMutex);
        QVector<ShpSimpleTask> taskList;
        for (auto&& v: coherentTaskList){
            std::visit([&taskList](auto&& cohTask){
                using T = std::decay_t<decltype (cohTask)>;
                if constexpr(std::is_same_v<T, ParallelCluster>){
                    // ParallelCluster& pc = std:
                    for (auto& pt: cohTask.parallelTasks){
                        taskList << pt;
                    }
                }
                else {
                    taskList << cohTask;
                }
            },v);

        }
        return taskList;
    }
    void run();
    void stop();

signals:
    void infoMsg(QString msg);
    void serviceMsg(QString msg);
    void finished();
private:
    const std::chrono::milliseconds checkNextTimeout{1};
    QMutex taskListMutex;
    TaskerState taskerState{TaskerState::idle};
    int coherentTasksCount{0};
    QPointer<QTimer> timer;
    QList<CoherentTask> coherentTaskList;
    int curTaskIdx{0};
    void runCoherentTask(CoherentTask &task);
    void checkForNextTask();
    int removeTaskFromCluster(const Shp<SimpleTask>& task, ParallelCluster& cluster){
        return cluster.parallelTasks.removeAll(task);
    }
};
struct VisitorRunner{
    void operator()(ParallelCluster& cluster){
        for (auto&& t: cluster.parallelTasks){
            t->run();
        }
        cluster.startTime = std::chrono::system_clock::now();
    }
    void operator()(Shp<SimpleTask>& task){
        task->run();
    }
};
#endif // SimpleTASKER_H
