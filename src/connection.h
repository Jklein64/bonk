#pragma once

#include <functional>
#include <string>
#include <vector>

struct Event {
    std::string event_type;
    std::string data;

    static Event from_audio_block(std::vector<double> audio_block);
    static Event from_viz_block(std::vector<double> viz_block);
    static Event from_heartbeat();

    std::string to_string() const;
};

class Connection {
  public:
    Connection(std::string client_id);

    inline ~Connection() {
        close_callback(*this);
    }

    void on_close(std::function<void(Connection&)> callback);
    void on_event(std::function<void(const Event&)> callback);
    bool send(const Event& event);

  private:
    // const std::unordered_map<std::string, std::weak_ptr<Connection>>& connections_ref;
    std::function<void(Connection&)> close_callback;
    std::function<void(const Event&)> event_callback;
    const std::string client_id;
};