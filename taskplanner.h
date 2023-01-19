#ifndef TASKPLANNER_H
#define TASKPLANNER_H
#include "taskcluster.h"

namespace ST {
struct SimpleBranch{
    bool isValid(){return first.isNull() || last.isNull();}
    ShpSimpleTask first;
    ShpSimpleTask last;
};
class TaskPlanner
{
public:
    static SimpleBranch bindCoherentBranch(const QVector<ShpSimpleTask>& tasks);
    static SimpleBranch bindCoherentBranch(const QVector<ShpSimpleTask>& firstPart, const QVector<ShpSimpleTask>& secondPart);
    static SimpleBranch bindCoherentBranch(const QVector<ShpSimpleTask>& firstPart, const ShpSimpleTask& secondPart);
    static ShpSimpleTask getLast(const ShpSimpleTask& target);
    static SimpleBranch bindCoherentBranch(const ShpSimpleTask& firstPart, const QVector<ShpSimpleTask>& secondPart);
    static SimpleBranch bindCoherentBranch(const ShpSimpleTask& firstPart, const ShpSimpleTask& secondPart);
    static QSharedPointer<SimulClusterTask> buildSimulCluster(const QVector<ShpSimpleTask>& tasks,
                                                    const QString& name);
    static bool insertAfter(ShpSimpleTask previous, ShpSimpleTask target);
    static ShpSimpleTask removeFromBranch(ShpSimpleTask head, ShpSimpleTask target);
    static int calcAllCount(const ShpSimpleTask& head);
    static int calcBranchCount(const ShpSimpleTask& head);
    static ShpSimpleTask detectLoop(const ShpSimpleTask &head);

};
}
#endif // TASKPLANNER_H
