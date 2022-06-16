#include "pausetask.h"

using namespace ST;

PauseTask::PauseTask(const std::chrono::milliseconds &pausetime, QObject &contextForTimer, const QString &name):
    SimpleTask(name), pausetime(pausetime), context(contextForTimer)
{

}
