#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "sim.h"

struct Event {
    std::string event_type;
    std::string data;

    static Event from_audio_block(std::vector<double> audio_block);
    static Event from_viz_block(std::vector<double> viz_block);
    static Event from_heartbeat();

    std::string to_string() const;
};

class EventStream {
  public:
    EventStream(std::string client_id);

    inline ~EventStream() {
        close_callback(*this);
    }

    void on_close(std::function<void(EventStream&)> callback);
    void on_event(std::function<void(const Event&)> callback);
    bool send(const Event& event);

  private:
    std::function<void(EventStream&)> close_callback;
    std::function<void(const Event&)> event_callback;
    const std::string client_id;
};

struct Connection {
    std::optional<EventStream> event_stream;
    std::optional<SimParams> params;
    std::optional<Sim> sim;
};