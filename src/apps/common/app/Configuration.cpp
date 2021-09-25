#include <Configuration.h>

#include <iostream>
#include <fstream>
#include <exception>

#include <unordered_map>

void WriteValuePairs(std::fstream& file, const std::unordered_map<std::string, std::string>& options)
{
    for(const auto& pair : options)
        file << pair.first << '=' << pair.second << '\n';
}

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
    file << "Width=" << config.window.resolutionW << '\n';
    file << "Height=" << config.window.resolutionH << '\n';
    file << "RefreshRate=" << config.window.refreshRate << '\n';
    file << "xPos=" << config.window.xPos << '\n';
    file << "yPos=" << config.window.yPos << '\n';
    file << "mode=" << config.window.mode << '\n';
    file << "vsync=" << config.window.vsync << '\n';
    file << "fov=" << (int)glm::degrees(config.window.fov) << '\n';
    file << '\n'; 

    file << "### Open GL Options ###" << '\n';
    file << "majorVersion=" << config.openGL.majorVersion << '\n';
    file << "minorVersion=" << config.openGL.minorVersion << '\n';
    file << "profileMask=" << config.openGL.profileMask << '\n';
    file << "antialiasing=" << config.openGL.antialiasing << '\n';
    file << "msaaSamples=" << config.openGL.msaaSamples << '\n';
    file << "shadersFolder=" << config.openGL.shadersFolder.string() << '\n';
    file << '\n';

    file << "### Logging Options ###" << '\n';
    file << "logFilePath=" << config.log.logFile.string() << '\n';
    file << "verbosity=" << static_cast<int>(config.log.verbosity) << '\n';
    file << '\n';

    file << "### " << config.window.name << " Options ###" << '\n';
    WriteValuePairs(file, config.options);
    file << '\n';
}

std::unordered_map<std::string, std::string> LoadKeyValuePairs(std::fstream& file)
{
    char line[255];
    std::unordered_map<std::string, std::string> keyValues;
    while(file.getline(line, 255))
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

int ExtractInt(std::unordered_map<std::string, std::string>& dictionary, std::string key)
{
    auto value = dictionary.at(key);
    dictionary.erase(key);
    return std::stoi(value);
}

template <typename T>
T ExtractEnum(std::unordered_map<std::string, std::string>& dictionary, std::string key)
{
    auto value = dictionary.at(key);
    dictionary.erase(key);
    return static_cast<T>(std::stoi(value));
}

float ExtractFloat(std::unordered_map<std::string, std::string>& dictionary, std::string key)
{
    auto value = dictionary.at(key);
    dictionary.erase(key);
    return static_cast<float>(std::stof(value));
}

std::string ExtractString(std::unordered_map<std::string, std::string>& dictionary, std::string key)
{
    auto value = dictionary.at(key);
    dictionary.erase(key);
    return value;
}

App::Configuration App::LoadConfig(std::filesystem::path filePath)
{
    std::fstream file{filePath, file.in};
    if(!file.is_open())
    {
        throw std::runtime_error("Could not open file " + filePath.string() + '\n');
    }

    App::Configuration config;

    config.options = LoadKeyValuePairs(file);

    config.window.name = ExtractString(config.options, "Name");
    config.window.resolutionW = ExtractInt(config.options, "Width"); 
    config.window.resolutionH = ExtractInt(config.options, "Height");
    config.window.refreshRate = ExtractInt(config.options, "RefreshRate");
    config.window.xPos = ExtractInt(config.options, "xPos");
    config.window.yPos = ExtractInt(config.options, "yPos");
    config.window.mode = ExtractEnum<App::Configuration::WindowMode>(config.options, "mode");
    config.window.vsync = ExtractInt(config.options, "vsync");
    config.window.fov = glm::radians(ExtractFloat(config.options, "fov"));

    config.openGL.majorVersion = ExtractInt(config.options, "majorVersion");
    config.openGL.minorVersion = ExtractInt(config.options, "minorVersion");
    config.openGL.profileMask = ExtractInt(config.options, "profileMask");
    config.openGL.antialiasing = ExtractInt(config.options, "antialiasing");
    config.openGL.msaaSamples = ExtractInt(config.options, "msaaSamples");
    config.openGL.shadersFolder = ExtractString(config.options, "shadersFolder");

    config.log.logFile = ExtractString(config.options, "logFilePath");
    config.log.verbosity = ExtractEnum<Log::Verbosity>(config.options, "verbosity");

    return config;
}