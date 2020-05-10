#ifndef KPINENTRY_LOGGING_HPP
#define KPINENTRY_LOGGING_HPP

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(LOG_KPINENTRY)

#define LOG_DEBUG(msg) qCDebug(LOG_KPINENTRY, "%s: " msg, __func__)
#define LOG_DEBUGF(format, ...) qCDebug(LOG_KPINENTRY, "%s: " format, __func__, __VA_ARGS__)
#define LOG_ERROR(msg) qCCritical(LOG_KPINENTRY, "%s: " msg, __func__)
#define LOG_ERRORF(format, ...) qCCritical(LOG_KPINENTRY, "%s: " format, __func__, __VA_ARGS__)

#endif // KPINENTRY_LOGGING_HPP
