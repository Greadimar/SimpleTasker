#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <random>
#include <iostream>
#include "../../simpletasker.h"
#include "../../pausetask.h"
#include "../../taskcluster.h"
struct RemoteWorker{
    std::random_device dev;
    using SuccessReply = std::function<void(QVector<uint>)>;
    using Fail = std::function<void()>;

    using answer = bool;
    using ComplexReply = std::function<void(answer, QVector<uint>)>;

    void acceptRequest(uint val, SuccessReply onSuccess, Fail onFail){
        qDebug() << "accept request:" << val << QTime::currentTime();
        QTimer::singleShot(1000, (qApp), [=](){
            std::uniform_int_distribution<std::mt19937::result_type> coin(1,10);
            std::mt19937 rng(dev());
            if (coin(rng)){
                onSuccess({0, 1, 2});
            }
            else{
                onFail();
            }
        });
    };
    void acceptRequest(uint val, ComplexReply onReply){
        qDebug() << "accept request with complex reply:" << val;
        QTimer::singleShot(1000, (qApp), [=](){
            std::uniform_int_distribution<std::mt19937::result_type> coin(0,10);
            std::mt19937 rng(dev());
            if (coin(rng)){
                onReply(true, {0, 1, 2});
            }
            else{
                onReply(false, {});
            }
        });
    };
    void acceptRequestWithNoReplyData(uint val, std::function<void(bool)> onReply){
        qDebug() << "accept request with complex reply:" << val;
        QTimer::singleShot(1000, (qApp), [=](){
            std::uniform_int_distribution<std::mt19937::result_type> coin(0,1);
            std::mt19937 rng(dev());
            if (coin(rng)){
                onReply(true);
            }
            else{
                onReply(false);
            }
        });
    };
};
using namespace ST;

using RemoteTask = SimpleReplyTask<QVector<uint>>;


class LocalWorker: public QObject{
    RemoteWorker remoteWorker;
    SimpleTasker tasker;
public:
    LocalWorker(){
        connect(&tasker, &SimpleTasker::finished, this, &LocalWorker::onFinish);
        connect(&tasker, &SimpleTasker::sendPrg, this, [=](int prg){
            qDebug() << "prg: " << prg;
        });
        connect(&tasker, &SimpleTasker::infoMsg, this, [=](QString msg){
            qDebug() << "simpletasker info: " << msg;
        });
    }
    ShpSimpleTask populateTree(){
        // making simple reply task;
        // we want to do the following tree of tasks:
                                  //A1 - A2 - A3 - A4 - B1 - B2
                                          //onfail:    \C1 - C2;
        // A will have replies
        // B will be simple execution + pause
        // C will be simultaneous cluster of tasks + pause

        auto shpRt = RemoteTask::create("A1");
        auto wkRt = shpRt.toWeakRef();
        shpRt->runner = [=](){
            remoteWorker.acceptRequest(1, wkRt.toStrongRef()->getOnSuccess(), wkRt.toStrongRef()->getOnFail());
        };
        shpRt->onSuccess = [=](QVector<uint> data){
             qDebug() << wkRt.toStrongRef()->getName() << "Success" << data;
        };
        shpRt->onFail = [=](){
            qDebug() << wkRt.toStrongRef()->getName() << "Fail";
        };
        // making simple reply task2, same as previous but with 3 tries to complete;
        auto shpRt2 = RemoteTask::create("A2  (with 3 tries)");
        auto wkRt2 = shpRt2.toWeakRef();
        shpRt2->runner = [=](){
            remoteWorker.acceptRequest(2, wkRt2.toStrongRef()->getOnSuccess(), wkRt2.toStrongRef()->getOnFail());
        };
        shpRt2->onSuccess = [=](QVector<uint> data){
            qDebug() << wkRt2.toStrongRef()->getName() << "Success" << data;
        };
        shpRt2->onFail = [=](){
            qDebug() << wkRt2.toStrongRef()->getName() << "Fail";
        };
        shpRt2->setMaximumTries(3);

        // making  task3, but it has only one callback to call in both success and fail cases.
        // like (true, {1,2,3} - for success and (false, {}) - for fail
        auto shpRt3 = RemoteTask::create("A3 (complex reply)");
        auto wkRt3 = shpRt3.toWeakRef();
        shpRt3->runner = [=](){
            remoteWorker.acceptRequest(3, wkRt3.toStrongRef()->getOnReply());
        };
        shpRt3->onSuccess = [=](QVector<uint> data){
            qDebug() << wkRt3.toStrongRef()->getName() << "Success" << data;
        };
        shpRt3->onFail = [=](){
            qDebug() << wkRt3.toStrongRef()->getName() << "Fail";
        };
        //  we want to use same signature for success but reply looks like void reply(bool success), so we can bind our own data to reply
        // can be done for getOnSuccessWithBindingArgs too
        auto shpRt4 = RemoteTask::create("A4 (reply with binding)");
        auto wkRt4 = shpRt4.toWeakRef();
        shpRt4->runner = [=](){
            remoteWorker.acceptRequestWithNoReplyData(3, wkRt4.toStrongRef()->getOnReplyWithBindedArgs(QVector<uint>{4,3,2,1}));
        };
        shpRt4->onSuccess = [=](QVector<uint> data){
            qDebug() << wkRt4.toStrongRef()->getName() << "Success" << data;
        };
        shpRt4->onFail = [=](){
            qDebug() << wkRt4.toStrongRef()->getName() << "Fail";
        };
        // building a branch A
        auto branchA = TaskPlanner::bindCoherentBranch({shpRt, shpRt2, shpRt3, shpRt4});

        //now we are making two separate branch to fork from branch A
        auto shpB1 = SimpleTask::create("B1 simple");
        shpB1->runner = [=](){
            qDebug() << "exec B1";
        };
        auto shpPauseB2 = PauseTask::create(std::chrono::seconds(1), *qApp, "B2 pause");
        // building branch B
        auto branchB = TaskPlanner::bindCoherentBranch(shpB1, shpPauseB2);


        // making 4 simultaneous tasks for cluster. Cluster is not complete untill every one of it's tasks is ready
        auto shpC1Simul = makeReplyTasks("c1-1");
        auto shpC2Simul = SimpleTask::create("c1-2");
        shpC2Simul->runner = [=](){
            qDebug() << "exec c1-2";
        };
        auto shpC3Simul = makeReplyTasks("c1-3");
        auto shpC4Simul = makeReplyTasks("c1-4");
        auto simulCluster = SimulClusterTask::create(
                    TaskPlanner::bindCoherentBranch({shpC1Simul, shpC2Simul, shpC3Simul, shpC4Simul})
                    , "Cluster C1");

        auto pauseC2 = PauseTask::create(std::chrono::milliseconds(400), *qApp, "c2 pause");

        //building branch C
        auto branchC = TaskPlanner::bindCoherentBranch(simulCluster, pauseC2);

        //connecting branches
        shpRt4->nextTask = branchB;
        shpRt4->nextOnFail = branchC;

        return branchA;
    }
    ShpSimpleTask makeReplyTasks(QString name){
        auto shpRt = RemoteTask::create(name);
        auto wkRt = shpRt.toWeakRef();
        shpRt->runner = [=](){
            remoteWorker.acceptRequest(10, wkRt.toStrongRef()->getOnSuccess(), wkRt.toStrongRef()->getOnFail());
        };
        shpRt->onSuccess = [=](QVector<uint> data){
             qDebug() << wkRt.toStrongRef()->getName() << "Success" << data << QTime::currentTime();
        };
        shpRt->onFail = [=](){
            qDebug() << wkRt.toStrongRef()->getName() << "Fail"  << QTime::currentTime();
        };
        return shpRt;
    }
    void run(){
        curRuns++;
        tasker.setFirstTask(populateTree());
        startTime = QTime::currentTime();
        tasker.run();
    }
    void run(std::chrono::milliseconds interval){
        tasker.setFirstTask(populateTree());
        tasker.runWithCommonInterval(interval);
    }
    QTime startTime;
    int curRuns{0};
    int maxRuns{10};
    void onFinish(){
        qDebug() << "finished in: " << startTime.msecsTo(QTime::currentTime()) << curRuns;
        if (curRuns < maxRuns) run();
    }
};


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    LocalWorker lw;
    lw.run();
    return a.exec();
}
