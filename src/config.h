#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QTypeInfo>
#include <QtGlobal>

class Config
{
public:
    Config(const QString& filePath = "./config/config.txt");

    static QString GetServerHostName(){
        return "172.25.244.247"; // WSL2 in my local machine
    }

    static quint16 getServerPort(){
        return 2345;
    }

private:
    QString m_configFilePath;
};

#endif // CONFIG_H
