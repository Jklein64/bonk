#include "connection.h"

#include <base64.hpp>
#include <fmt/core.h>

Event Event::from_heartbeat() {
    return {
        .event_type = "heartbeat",
        .data = "",
    };
}

Event Event::from_audio_block(std::vector<double> audio_block) {
    // Fine as long as server is known little-endian and client parses that way too
    const char* buffer = reinterpret_cast<const char*>(audio_block.data());
    return {
        .event_type = "audio-block",
        .data = base64::encode_into<std::string>(&buffer[0], &buffer[audio_block.size() * sizeof(double)]),
    };
}

Event Event::from_viz_block(std::vector<double> viz_block) {
    // TODO
    return {
        .event_type = "viz-block",
        .data = "",
    };
}

std::string Event::to_string() const {
    // See https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events#event_stream_format
    return fmt::format("event: {}\ndata: {}\n\n", this->event_type, this->data);
}