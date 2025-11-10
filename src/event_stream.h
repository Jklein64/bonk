#pragma once

#include <condition_variable>
#include <functional>
#include <queue>
#include <string>
#include <vector>

struct Event {
    std::optional<std::string> id;
    std::string event_type;
    std::string data;

    static Event from_audio_block(std::vector<float> audio_block, size_t sample_idx);
    static Event from_viz_block(std::vector<float> viz_block, size_t sample_idx);
    static Event from_heartbeat();

    std::string to_string() const;
};

class EventStream {
  public:
    void on_event(std::function<bool(const Event&)> callback);
    void send(const Event& event);

  private:
    std::queue<Event> message_queue;
    std::condition_variable cv;
    std::mutex mutex;
};