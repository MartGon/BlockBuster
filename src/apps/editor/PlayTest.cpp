#include <PlayTest.h>

#include <cstring>
#include <fstream>
#include <vector>

#ifndef _WIN32
    #include <fcntl.h>
    #include <signal.h>
    #include <spawn.h>
    #include <sys/wait.h>
    #include <unistd.h>

    extern char** environ;
#endif

using namespace BlockBuster::Editor;

PlayTest::~PlayTest()
{
    Stop();
}

bool PlayTest::IsSupported()
{
#ifdef _WIN32
    return false;
#else
    return true;
#endif
}

std::optional<std::string> PlayTest::Launch(Params params, std::filesystem::path script, std::filesystem::path mapsFolder)
{
#ifdef _WIN32
    return "Play test is not supported on Windows yet";
#else
    Poll();
    if(state == State::RUNNING)
        return "A play test is already running";

    std::error_code ec;
    if(!std::filesystem::is_regular_file(script, ec))
        return "Play test script not found: " + script.string();

    // The script loads maps from <repo>/maps, next to its own folder
    auto root = std::filesystem::weakly_canonical(std::filesystem::absolute(script).parent_path() / "..", ec);
    auto scriptMaps = std::filesystem::weakly_canonical(root / "maps", ec);
    auto editorMaps = std::filesystem::weakly_canonical(std::filesystem::absolute(mapsFolder), ec);
    if(scriptMaps != editorMaps)
        return "The play test only works with maps in " + scriptMaps.string() + ". The editor's maps folder is " + editorMaps.string();

    auto logsFolder = root / "logs";
    std::filesystem::create_directories(logsFolder, ec);
    logFile = logsFolder / "playtest.out";

    // Redirect the script's output to the log file
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, logFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);
    posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);

    auto scriptPath = std::filesystem::absolute(script).string();
    auto bots = std::to_string(params.bots);
    std::vector<std::string> args = {"bash", scriptPath, "-m", params.map, "-b", bots, "-D", params.difficulty, "-g", params.mode};
    std::vector<char*> argv;
    for(auto& arg : args)
        argv.push_back(arg.data());
    argv.push_back(nullptr);

    pid_t child;
    int res = posix_spawnp(&child, "bash", &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    if(res != 0)
        return "Could not start bash: " + std::string{strerror(res)};

    pid = child;
    state = State::RUNNING;
    error.clear();

    return std::nullopt;
#endif
}

void PlayTest::Stop()
{
#ifndef _WIN32
    if(state != State::RUNNING)
        return;

    // The script's trap stops the client and the server
    kill(pid, SIGTERM);
    int status;
    waitpid(pid, &status, 0);
    pid = -1;
    state = State::IDLE;
#endif
}

void PlayTest::Poll()
{
#ifndef _WIN32
    if(state != State::RUNNING)
        return;

    int status;
    if(waitpid(pid, &status, WNOHANG) != pid)
        return;

    pid = -1;
    bool ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    if(ok)
        state = State::IDLE;
    else
    {
        state = State::FAILED;
        error = ReadLastLogLine();
        if(error.empty())
            error = "The script exited with an error";
        error += " (see " + logFile.string() + ")";
    }
#endif
}

std::string PlayTest::ReadLastLogLine()
{
    std::ifstream file{logFile};
    std::string line, last;
    while(std::getline(file, line))
        if(!line.empty())
            last = line;

    return last;
}
