#include <doctest/doctest.h>

#include "ipc_commands.hpp"

TEST_CASE("parse_ipc_command parses pause/resume") {
    {
        auto cmd = wp::parse_ipc_command("pause");
        CHECK(cmd.type == wp::IPCCommandType::Pause);
        CHECK(cmd.argument.empty());
    }
    {
        auto cmd = wp::parse_ipc_command(" resume \r\n");
        CHECK(cmd.type == wp::IPCCommandType::Resume);
        CHECK(cmd.argument.empty());
    }
}

TEST_CASE("parse_ipc_command parses set-video and set-shader with paths") {
    {
        auto cmd = wp::parse_ipc_command("set-video C:\\wallpapers\\a.mp4");
        CHECK(cmd.type == wp::IPCCommandType::SetVideo);
        CHECK(cmd.argument == "C:\\wallpapers\\a.mp4");
    }
    {
        auto cmd = wp::parse_ipc_command("  set-shader  shaders/plasma.glsl  ");
        CHECK(cmd.type == wp::IPCCommandType::SetShader);
        CHECK(cmd.argument == "shaders/plasma.glsl");
    }
}

TEST_CASE("parse_ipc_command parses list/status/stop") {
    CHECK(wp::parse_ipc_command("status").type == wp::IPCCommandType::GetStatus);
    CHECK(wp::parse_ipc_command("get-status").type == wp::IPCCommandType::GetStatus);
    CHECK(wp::parse_ipc_command("list").type == wp::IPCCommandType::ListWallpapers);
    CHECK(wp::parse_ipc_command("list-wallpapers").type == wp::IPCCommandType::ListWallpapers);
    CHECK(wp::parse_ipc_command("stop").type == wp::IPCCommandType::Stop);
    CHECK(wp::parse_ipc_command("quit").type == wp::IPCCommandType::Stop);
}

TEST_CASE("parse_ipc_command rejects missing arguments") {
    CHECK(wp::parse_ipc_command("set-video").type == wp::IPCCommandType::Unknown);
    CHECK(wp::parse_ipc_command("set-shader   ").type == wp::IPCCommandType::Unknown);
}


