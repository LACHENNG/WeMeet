#include "config.h"
#include "qdebug.h"
#include <QTimeZone>
#include <filesystem>

Config::Config(const std::string &filename)
{
    std::ifstream file(filename);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            // skip empty line and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }
            // parse key:value
            std::string key, value;
            bool colon = false;
            for (char c : line) {
                if (c == ':') {
                    colon = true;
                    continue;
                }
                if (colon) {
                    value += c;
                } else {
                    key += c;
                }
            }
            // trim blank of key and value
            key = trim(key);
            value = trim(value);
            // store
            data.insert({key, value});
        }
        file.close();
    } else {
        // can`t open file

        qDebug() << std::filesystem::absolute(filename).string().c_str() << "  ;;;";
        throw std::runtime_error("Cannot open config file: [" + filename + "] make sure it is at the same directory as the binary file");
    }
}

//Config::Config(const QString &filePath)
//{
////    QTimeZone::setSystemTimeZone("Asia/Shanghai");

//    //    QTimeZone::systemTimeZone();
//}

std::string Config::trim(const std::string &s)
{
    int start = 0, end = s.size() - 1;
    while (start < s.size() && s[start] == ' ') {
        start++;
    }
    while (end >= 0 && s[end] == ' ') {
        end--;
    }
    return s.substr(start, end - start + 1);
}

std::string Config::get(const std::string &key, const std::string &default_value) {
    if (data.count(key) > 0) {
        return data[key];
    } else {
        return default_value;
    }
}

void Config::set(const std::string &key, const std::string &value) {
    if (data.count(key) > 0) {
        data[key] = value;
    } else {
        data.insert({key, value});
    }
}

void Config::print() {
    for (auto& pair : data) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
}

