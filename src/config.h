#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QTypeInfo>
#include <QtGlobal>

class Config
{
private:
    Config(const QString& filePath = "./config/config.txt");

    Config(const Config& that) = delete;
    Config& operator=(const Config& that) = delete;

public:
    // singleton, do not delete
    static Config& getInstance(){
        static Config config;
        return config;
    }

    QString GetServerHostName(){
        return "172.29.167.72"; // WSL2 in my local machine
    }

    quint16 getServerPort(){
        return 2345;
    }

    std::string userName(){
        return "chenlang";
    }

    std::string userId(){
        return "12255643";
    }

    auto cameraIdx(){
        return 0;
    }

private:
    QString m_configFilePath;
};

#endif // CONFIG_H
