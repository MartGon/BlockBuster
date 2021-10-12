#pragma once

#include <stdint.h>
#include <optional>
#include <functional>
#include <unordered_map>

#include <Peer.h>
#include <Address.h>

namespace ENet
{
    // TODO: Consider storing peers in a vector. They are bound to the host instance after all
    // Allow the user to set some callbacks to handle different events
    // After that, user simply calls PollEvent to dispatch each of the events

    using PeerId = uint8_t;

    struct PeerInfo
    {
        Address address;
        uint32_t roundTripTimeMs; // In milliseconds
    };

    class Host
    {
    friend class HostFactory;
    public:
        ~Host();

        Host(const Host&) = delete;
        Host& operator=(const Host&) = delete;

        Host(Host&&);
        Host& operator=(Host&&);

        void SetOnRecvCallback(std::function<void(PeerId, uint8_t, RecvPacket)> callback);
        void SetOnConnectCallback(std::function<void(PeerId)> callback);
        void SetOnDisconnectCallback(std::function<void(PeerId)> callback);

        // Wait a maximum of timeout (ms) until an event is recv
        bool PollEvent(uint32_t timeout = 0);
        // Polls all events until none is recv or timeout (ms) elapses
        void PollAllEvents(uint32_t timeout = -1);

        void Connect(Address address);
        void SendPacket(PeerId id, uint8_t channelId, const SentPacket& packet);
        void Broadcast(uint8_t channelId, const SentPacket& packet);

        PeerInfo GetPeerInfo(PeerId id) const;

    private:
        Host(Address address, uint32_t connections, uint32_t channels, uint32_t inBandwidth, uint32_t outBandwidth);
        Host(uint32_t connections, uint32_t channels, uint32_t inBandwidth, uint32_t outBandwidth);

        const Peer& GetPeer(PeerId id) const;
        Peer& GetPeer(PeerId id);
        PeerId AddPeer(ENetPeer* peer);
        PeerId RemovePeer(ENetPeer* peer);
        void CheckPeer(PeerId id) const;
        PeerId GetPeerId(ENetPeer* peer) const;
        PeerId GetFreePeerId() const;

        std::function<void(PeerId, uint8_t, RecvPacket)> onRecv_ = {};
        std::function<void(PeerId)> onConnect_ = {};
        std::function<void(PeerId)> onDisconnect_ = {};

        std::unordered_map<PeerId, ENet::Peer> peers_;

        Address address_;
        uint32_t connections_;
        uint32_t channels_;
        uint32_t inBandwidth_;
        uint32_t outBandwidth_;
        ENetHost* socket_ = nullptr;
    };
}