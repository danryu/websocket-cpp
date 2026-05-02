#pragma once
#include <queue>
#include <span>
#include <vector>

#include "macros/autoptr.hpp"
#include "util/critical.hpp"
#include "util/prependable-buffer.hpp"

// libwebsockets
extern "C" {
struct lws_context;
struct lws;
auto lws_context_destroy(lws_context* context) -> void;
}

namespace ws::impl {
struct OutgoingPacket {
    PrependableBuffer data;
    bool              text;
};
using SendBuffers = Critical<std::queue<OutgoingPacket>>;

declare_autoptr(LWSContext, lws_context, lws_context_destroy);

auto append_payload(lws* wsi, PrependableBuffer& buffer, void* const in, const size_t len) -> bool; // returns true if final
auto push_to_send_buffers_and_cancel_service(SendBuffers& send_buffers, PrependableBuffer buffer, bool text, lws* wsi) -> void;
auto send_one_from_send_buffer(SendBuffers& send_buffers, lws* wsi) -> bool;
} // namespace ws::impl
