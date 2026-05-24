// The prebuilt libwebsockets we link against (vcpkg/MSVC) is a release build (/MD,
// NDEBUG). Force this translation unit's <libwebsockets.h> into release mode so the
// header's inline code and any assert()s match the library's CRT/ABI; without this a
// debug build (_DEBUG / /MDd) hits _ITERATOR_DEBUG_LEVEL and heap-ABI mismatches.
// Scoped to the libwebsockets include only -- keep this block immediately before it.
#ifndef NDEBUG
#define NDEBUG
#endif
#undef _DEBUG
#include <libwebsockets.h>

#include "impl.hpp"
#include "macros/assert.hpp"

namespace ws::impl {
namespace {
auto push_to_send_buffers(SendBuffers& send_buffers, PrependableBuffer buffer, const bool text) -> void {
    buffer.enlarge_forward(LWS_SEND_BUFFER_PRE_PADDING);
    buffer.enlarge(LWS_SEND_BUFFER_POST_PADDING);
    auto [lock, buf] = send_buffers.access();
    buf.push({std::move(buffer), text});
}
} // namespace

auto append_payload(lws* wsi, PrependableBuffer& buffer, void* const in, const size_t len) -> bool {
    std::memcpy(buffer.enlarge(len).data(), in, len);
    const auto remaining = lws_remaining_packet_payload(wsi);
    const auto final     = lws_is_final_fragment(wsi);
    return remaining == 0 && final;
}

auto push_to_send_buffers_and_cancel_service(SendBuffers& send_buffers, PrependableBuffer buffer, const bool text, lws* const wsi) -> void {
    push_to_send_buffers(send_buffers, buffer, text);
    lws_callback_on_writable(wsi);
    lws_cancel_service_pt(wsi);
}

auto send_one_from_send_buffer(SendBuffers& send_buffers, lws* const wsi) -> bool {
    auto packet = OutgoingPacket();
    auto empty  = false;
    {
        auto [lock, buf] = send_buffers.access();
        if(buf.empty()) {
            return true;
        }
        packet = std::move(buf.front());
        buf.pop();
        empty = buf.empty();
    }
    const auto body = packet.data.body();
    const auto head = body.data() + LWS_SEND_BUFFER_PRE_PADDING;
    const auto size = body.size() - LWS_SEND_BUFFER_PRE_PADDING - LWS_SEND_BUFFER_POST_PADDING;
    const auto ret  = lws_write(wsi, std::bit_cast<unsigned char*>(head), size, packet.text ? LWS_WRITE_TEXT : LWS_WRITE_BINARY);
    ensure(ret >= int(size));
    if(!empty) {
        lws_callback_on_writable(wsi);
    }
    return true;
}
} // namespace ws::impl
