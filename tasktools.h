#ifndef TASKTOOLS_H
#define TASKTOOLS_H
#include <QSharedPointer>
namespace TaskTools{

using timepoint = std::chrono::time_point<std::chrono::system_clock>;

template <typename Base>
inline QSharedPointer<Base>
shared_from_base(QEnableSharedFromThis<Base>* base)
{
    return base->sharedFromThis();
}
template <typename Base>
inline QSharedPointer<const Base>
shared_from_base(QEnableSharedFromThis<Base> const* base)
{
    return base->sharedFromThis();
}
template <typename That>
inline QSharedPointer<That>
shared_from(That* that)
{
    auto&& shb = shared_from_base(that);
    return shb.template staticCast<That>();
}

}
#endif // TASKTOOLS_H
