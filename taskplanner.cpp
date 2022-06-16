#include "taskplanner.h"
#include <unordered_set>
using namespace ST;
ShpSimpleTask TaskPlanner::bindCoherentBranch(const QVector<ShpSimpleTask> &tasks){
    if (tasks.isEmpty()) return nullptr;
    if (tasks.size() < 2) return tasks.first();
    auto it = tasks.begin();
    auto curTask = *it;
    it++;
    auto nextTask = *it;
    while (it != tasks.end()){
        nextTask = *it;
        bindCoherentBranch(curTask, nextTask);
        //curTask->nextTask = nextTask;
        curTask = nextTask;
        it++;
    }
    return tasks.first();
}

ShpSimpleTask TaskPlanner::bindCoherentBranch(const QVector<ShpSimpleTask> &firstPart, const QVector<ShpSimpleTask> &secondPart){
    if (firstPart.isEmpty() || secondPart.isEmpty()) return nullptr;
    if (!firstPart.last() || !secondPart.first()) return nullptr;
    firstPart.last()->nextTask = secondPart.first();
    return firstPart.first();
}

ShpSimpleTask TaskPlanner::bindCoherentBranch(const QVector<ShpSimpleTask> &firstPart, const ShpSimpleTask &secondPart){
    if (firstPart.isEmpty()) return nullptr;
    if (!firstPart.last() || !secondPart) return nullptr;
    firstPart.last()->nextTask = secondPart;
    return firstPart.first();
}

ShpSimpleTask TaskPlanner::getLast(const ShpSimpleTask &target){
    if (!target) return nullptr;
    auto cur = target;
    while (cur->nextTask){
        cur = cur->nextTask;
    }
    return cur;
}

ShpSimpleTask TaskPlanner::bindCoherentBranch(const ShpSimpleTask &firstPart, const QVector<ShpSimpleTask> &secondPart){
    if (secondPart.isEmpty()) return nullptr;
    if (!firstPart || !secondPart.first()) return nullptr;
    auto cur = firstPart;
    while (cur->nextTask){
        cur = cur->nextTask;
    }
    cur->nextTask = secondPart.first();
    return firstPart;
}

ShpSimpleTask TaskPlanner::bindCoherentBranch(const ShpSimpleTask &firstPart, const ShpSimpleTask &secondPart){
    if (!firstPart || !secondPart) return nullptr;
    auto cur = firstPart;
    while (cur->nextTask){
        cur = cur->nextTask;
    }
    cur->nextTask = secondPart;
    return firstPart;
}

QSharedPointer<SimulClusterTask> TaskPlanner::buildSimulCluster(const QVector<ShpSimpleTask> &tasks, const QString &name){
    auto cluster = SimulClusterTask::create(tasks.first(), name);
    bindCoherentBranch(tasks);
    return cluster;
}

bool TaskPlanner::insertAfter(ShpSimpleTask previous, ShpSimpleTask target){
    if (!previous) return false;
    target->nextTask = previous->nextTask;
    previous->nextTask = target->nextTask;
    return true;
}

ShpSimpleTask TaskPlanner::removeFromBranch(ShpSimpleTask head, ShpSimpleTask target){
    if (!head || !target) return head;
    if (head == target){
        return head->nextTask;
    }
    auto cur = head;
    ShpSimpleTask prev;
    QSet<ShpSimpleTask> loopCheckSet;
    while (cur != nullptr){
        if (loopCheckSet.find(cur) != loopCheckSet.end()) return head;
        loopCheckSet.insert(cur);
        prev = cur;
        cur = cur->nextTask;
        if (cur == target){
            prev->nextTask = cur->nextTask;
        }
    }
    return head;
}

int TaskPlanner::calcAllCount(const ShpSimpleTask &head){
    if (!head) return 0;
    auto cur = head;
    int count{0};
    QSet<ShpSimpleTask> loopSet;
    while (cur != nullptr){
        if (loopSet.find(cur) != loopSet.end()) return count;
        loopSet.insert(cur);
        count++;
        if (cur->nextOnFail){
            count += calcAllCount(cur->nextOnFail);
        }
        cur = cur->nextTask;

    }
    return count;
}

int TaskPlanner::calcBranchCount(const ShpSimpleTask &head){
    int counter{0};
    auto task = head;
    while(task){
        counter++;
        task = task->nextTask;
    }
    return counter;
}

ShpSimpleTask TaskPlanner::detectLoop(const ShpSimpleTask& head){
    if (!head) return head;;
    auto cur = head;
    QSet<ShpSimpleTask> loopSet;
    while (cur != nullptr){
        if (loopSet.find(cur) != loopSet.end()) return cur;
        loopSet.insert(cur);
        cur = cur->nextTask;
    }
    return cur;
}
