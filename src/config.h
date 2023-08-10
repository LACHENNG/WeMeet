// Author : lang @ nwpu
// All rights reserved

#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>
#include <fstream>
#include <string>
#include <map>

// Used to store and load key:values in config.txt
// Also support set values at runtime
// It is a singleton
class Config {
private:
    Config(const std::string& filename);
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
public:
    static Config& getInstance(){
        static Config config("config.txt");
        return config;
    }

private:
    std::map<std::string, std::string> data;

public:
    // trim blanks of strings
    std::string trim(const std::string& s);

    // get a value of key, return default value if not exist
    std::string get(const std::string& key, const std::string& default_value = "");

    // set value of a key ( existed one will be updated, insert otherwise)
    void set(const std::string& key, const std::string& value);

    // print all key:value stored or setted
    // useful for debuging
    void print();
};

#endif // CONFIG_H
