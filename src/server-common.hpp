#pragma once
#include <vector>

#include "common.hpp"
#include "impl.hpp"

namespace ws::server {
template <class T>
struct SessionDataInitializerCommon {
    virtual auto alloc(T* client) -> void* = 0;
    virtual auto free(void* ptr) -> void   = 0;

    virtual ~SessionDataInitializerCommon() {}
};

template <class T>
using OnDataReceivedCommon = void(T* client, PrependableBuffer payload);

enum class State {
    Connected,
    Destroyed,
};

struct ContextParams {
    const char*     protocol;
    const char*     cert        = nullptr;
    const char*     private_key = nullptr;
    int             port;
    KeepAliveParams keepalive = {};
};

struct ContextCommon {
    // private
    impl::AutoLWSContext context;
    PrependableBuffer    receive_buffer;
    State                state;

    auto init_protocol(const ContextParams& params, size_t session_data_size, const void* session_callback) -> bool;

    // public
    bool dump_packets = false;

    auto process() -> bool;
    auto shutdown() -> void;
};
} // namespace ws::server
