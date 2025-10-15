#include "event_stream.h"

#include <base64.hpp>
#include <fmt/core.h>

Event Event::from_heartbeat() {
    return Event("heartbeat", "");
}

Event Event::from_audio_block(std::vector<double> audio_block) {
    // Fine as long as server is known little-endian and client parses that way too
    const char* ptr = reinterpret_cast<const char*>(audio_block.data());
    std::string buffer = base64::encode_into<std::string>(&ptr[0], &ptr[audio_block.size() * sizeof(double)]);
    return Event("audio-block", buffer);
}

Event Event::from_viz_block(std::vector<double> viz_block) {
    // TODO
    return Event("viz-block", "");
}

std::string Event::to_string() const {
    // See https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events#event_stream_format
    return fmt::format("event: {}\ndata: {}\n\n", this->event_type, this->data);
}

void EventStream::send(const Event& event) {
    std::unique_lock<std::mutex> lk(mutex);
    message_queue.push(event);
    cv.notify_all();
}

void EventStream::on_event(std::function<bool(const Event&)> callback) {
    std::unique_lock<std::mutex> lk(mutex);
    while (this->message_queue.empty()) {
        cv.wait(lk);
    }

    while (!this->message_queue.empty()) {
        const Event& e = this->message_queue.front();
        if (!callback(e)) {
            // Stop writing if a write failed
            return;
        }
        this->message_queue.pop();
    }
}