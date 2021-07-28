#include <Configuration.h>

#include <iostream>
#include <fstream>
#include <exception>

#include <unordered_map>

void App::WriteConfig(App::Configuration config, std::filesystem::path filePath)
{
    std::fstream file{filePath, file.out};
    if(!file.is_open())
    {
        std::cout << "Could not open file " << filePath << '\n';
        return;
    }

    file << "### App Configuration ###" << '\n';
    file << "\n";

    file << "### Window Options ###" << '\n';
    file << "Name=" << config.window.name << '\n';
    file << "Width=" << config.window.width << '\n';
    file << "Height=" << config.window.height << '\n';
    file << "xPos=" << config.window.xPos << '\n';
    file << "yPos=" << config.window.yPos << '\n';
    file << "mode=" << config.window.mode << '\n';
    file << "vsync=" << config.window.vsync << '\n';
    file << '\n'; 

    file << "### Open GL Options ###" << '\n';
    file << "majorVersion=" << config.openGL.majorVersion << '\n';
    file << "minorVersion=" << config.openGL.minorVersion << '\n';
    file << "profileMask=" << config.openGL.profileMask << '\n';
    file << '\n';
}

std::unordered_map<std::string, std::string> LoadKeyValuePairs(std::fstream& file)
{
    char line[32];
    std::unordered_map<std::string, std::string> keyValues;
    while(file.getline(line, 32))
    {
        std::string strLine{line};
        auto index = strLine.find('=');
        if(index != strLine.npos)
        {
            std::string prefix = strLine.substr(0, index);
            std::string suffix = strLine.substr(index + 1, strLine.size());
            keyValues[prefix] = suffix;
        }
    }

    return keyValues;
}

template <typename T = int>
T GetIntType(const std::unordered_map<std::string, std::string>& dictionary, std::string key)
{
    return static_cast<T>(std::stoi(dictionary.at(key)));
}

App::Configuration App::LoadConfig(std::filesystem::path filePath)
{
    std::fstream file{filePath, file.in};
    if(!file.is_open())
    {
        throw std::runtime_error("Could not open file " + filePath.string() + '\n');
    }

    auto dictionary = LoadKeyValuePairs(file);

    App::Configuration config;

    config.window.name = dictionary.at("Name");
    config.window.width = GetIntType(dictionary, "Width");
    config.window.height = GetIntType(dictionary, "Height");
    config.window.xPos = GetIntType(dictionary, "xPos");
    config.window.yPos = GetIntType(dictionary, "yPos");
    config.window.mode = GetIntType<App::Configuration::WindowMode>(dictionary, "mode");
    config.window.vsync = GetIntType(dictionary, "vsync");

    config.openGL.majorVersion = GetIntType(dictionary, "majorVersion");
    config.openGL.minorVersion = GetIntType(dictionary, "minorVersion");
    config.openGL.minorVersion = GetIntType(dictionary, "profileMask");

    return config;
}