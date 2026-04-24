#include "lua_engine.hpp"
#include "logger.hpp"
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <filesystem>
#include <windows.h>

namespace wp {

LuaEngine::LuaEngine() {
    L = luaL_newstate();
    luaL_openlibs(L);
}

LuaEngine::~LuaEngine() {
    if (L) lua_close(L);
}

void LuaEngine::init() {
    register_api();
    WP_INFO("Lua engine initialized.");
}

void LuaEngine::register_api() {
    // wp.set_video(path)
    lua_newtable(L);
    
    lua_pushcfunction(L, [](lua_State* L) -> int {
        const char* path = luaL_checkstring(L, 1);
        WP_INFO("Lua requested video change: {}", path);
        // This will need to call back into the main engine
        return 0;
    });
    lua_setfield(L, -2, "set_video");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        SYSTEM_POWER_STATUS status;
        GetSystemPowerStatus(&status);
        lua_pushboolean(L, status.ACLineStatus == 0);
        return 1;
    });
    lua_setfield(L, -2, "is_on_battery");

    lua_setglobal(L, "wp");
}

void LuaEngine::execute_file(const std::string& path) {
    if (!std::filesystem::exists(path)) return;
    
    if (luaL_dofile(L, path.c_str()) != LUA_OK) {
        WP_ERROR("Lua error in {}: {}", path, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

void LuaEngine::on_event(const std::string& event_name) {
    std::string path = "scripts/" + event_name + ".lua";
    execute_file(path);
}

} // namespace wp
