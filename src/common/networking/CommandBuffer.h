#pragma once

#include <Command.h>

#include <enetw/ENetW.h>

#include <unordered_map>
#include <util/Queue.h>

namespace Networking
{
    class CommandBuffer
    {
    public:
        CommandBuffer(uint32_t capacity);

        std::optional<Command> Pop(ENet::PeerId peerId, Command::Type type);
        void Push(ENet::PeerId peerId, Command command);

        std::optional<Command> GetLast(ENet::PeerId peerId, Command::Type type);
        std::optional<Command> GetAt(ENet::PeerId peerId, Command::Type type, int32_t index);
        std::vector<Command> GetBy(ENet::PeerId peerId, Command::Type type, std::function<bool(Command)> predicate);
        std::optional<Command> GetFirstBy(ENet::PeerId peerId, Command::Type type, std::function<bool(Command)> predicate);
        std::optional<Command> GetLastBy(ENet::PeerId peerId, Command::Type type, std::function<bool(Command)> predicate);

    private:

        Util::Queue<Command>& GetQueue(ENet::PeerId peerId, Command::Type type);

        using Buffer = std::unordered_map<Command::Type, Util::Queue<Command>>;
        std::unordered_map<ENet::PeerId, Buffer> table_;

        uint32_t capacity_ = 0;
    };
}