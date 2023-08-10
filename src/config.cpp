#include "config.h"
#include <QTimeZone>


Config::Config(const QString &filePath)
{
//    QTimeZone::setSystemTimeZone("Asia/Shanghai");

    //    QTimeZone::systemTimeZone();
}

void Config::setUserId(const std::string &newUserId)
{
    m_userId = newUserId;
}

void Config::setUserName(const std::string &newUserName)
{
    m_userName = newUserName;
}



