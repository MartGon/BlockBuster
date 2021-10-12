#include <CommandBuffer.h>

using namespace Networking;

CommandBuffer::CommandBuffer(uint32_t capacity) : capacity_{capacity}
{

}

void CommandBuffer::Push(ENet::PeerId peerId, Command command)
{
    auto& queue = GetQueue(peerId, command.header.type);
    queue.Push(command);
}

std::optional<Command> CommandBuffer::GetLast(ENet::PeerId peerId, Command::Type type)
{
    return GetAt(peerId, type, -1);
}

std::optional<Command> CommandBuffer::GetAt(ENet::PeerId peerId, Command::Type type, int32_t index)
{
    auto& queue = GetQueue(peerId, type);
    return queue.At(index);
}

std::optional<Command> CommandBuffer::GetFirst(ENet::PeerId peerId, Command::Type type, std::function<bool(Command)> pred)
{
    std::optional<Command> ret;
    auto commands = Get(peerId, type, pred);
    if(commands.size() > 0)
        ret = commands[0];
    
    return ret;
}

std::optional<Command> CommandBuffer::GetLast(ENet::PeerId peerId, Command::Type type, std::function<bool(Command)> pred)
{
    std::optional<Command> ret;
    auto commands = Get(peerId, type, pred);
    if(commands.size() > 0)
        ret = commands[commands.size() - 1];
    
    return ret;
}

std::vector<Command> CommandBuffer::Get(ENet::PeerId peerId, Command::Type type, std::function<bool(Command)> pred)
{
    std::vector<Command> commands;

    auto& queue = GetQueue(peerId, type);
    for(auto i = 0; i < queue.GetSize(); i++)
    {
        auto command = queue.At(i).value();
        if(pred(command))
            commands.push_back(command);
    }

    return commands;
}

Util::Queue<Command>& CommandBuffer::GetQueue(ENet::PeerId peerId, Command::Type type)
{
    auto& buffer = table_[peerId];
    if(buffer.find(type) == buffer.end())
        buffer.emplace(std::piecewise_construct, std::forward_as_tuple(type), std::forward_as_tuple(5));

    return buffer[type];
}