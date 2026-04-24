#pragma once
#include <string>

struct lua_State;

namespace wp {

class LuaEngine {
public:
    LuaEngine();
    ~LuaEngine();

    void init();
    void execute_file(const std::string& path);
    void on_event(const std::string& event_name);

private:
    void register_api();
    lua_State* L = nullptr;
};

} // namespace wp
