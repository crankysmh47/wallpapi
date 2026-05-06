#include <doctest/doctest.h>

#include "config.hpp"

#include <filesystem>
#include <fstream>
#include <string>

static std::filesystem::path write_temp_file(const std::string& name, const std::string& contents) {
    auto dir = std::filesystem::temp_directory_path() / "wallpapi-tests";
    std::filesystem::create_directories(dir);
    auto p = dir / name;
    std::ofstream out(p, std::ios::binary);
    out << contents;
    out.close();
    return p;
}

TEST_CASE("ConfigManager loads basic fields") {
    // Create a fake project layout next to the config file so we can validate
    // relative-path resolution.
    auto dir = std::filesystem::temp_directory_path() / "wallpapi-tests" / "config-basic";
    std::filesystem::create_directories(dir / "wallpapers");
    std::filesystem::create_directories(dir / "shaders");

    // Dummy files (existence is not required for resolution, but helps debugging).
    {
        std::ofstream(dir / "wallpapers" / "levi.mp4", std::ios::binary).close();
        std::ofstream(dir / "shaders" / "a.glsl", std::ios::binary).close();
    }

    const auto cfgPath = dir / "config.toml";
    {
        std::ofstream out(cfgPath, std::ios::binary);
        out
            << "video = \"wallpapers/levi.mp4\"\n"
            << "shader = \"shaders/a.glsl\"\n"
            << "muted = false\n"
            << "fps_limit = 42\n"
            << "pause_on_battery = false\n"
            << "pause_on_fullscreen = true\n";
    }

    wp::ConfigManager cm;
    REQUIRE(cm.load(cfgPath.string()));

    // Paths should be resolved relative to config directory.
    CHECK(std::filesystem::path(cm.get_current().video_path).is_absolute());
    CHECK(std::filesystem::path(cm.get_current().shader_path).is_absolute());

    CHECK(std::filesystem::path(cm.get_current().video_path).filename() == "levi.mp4");
    CHECK(std::filesystem::path(cm.get_current().shader_path).filename() == "a.glsl");

    CHECK(cm.get_current().muted == false);
    CHECK(cm.get_current().fps_limit == 42);
    CHECK(cm.get_current().pause_on_battery == false);
    CHECK(cm.get_current().pause_on_fullscreen == true);
}

TEST_CASE("ConfigManager persists last-used wallpaper") {
    auto dir = std::filesystem::temp_directory_path() / "wallpapi-tests" / "config-persist";
    std::filesystem::create_directories(dir);

    const auto cfgPath = dir / "config.toml";
    {
        std::ofstream out(cfgPath, std::ios::binary);
        out
            << "video = \"wallpapers/old.mp4\"\n"
            << "shader = \"shaders/old.glsl\"\n"
            << "muted = true\n"
            << "fps_limit = 60\n"
            << "pause_on_battery = true\n"
            << "pause_on_fullscreen = true\n";
    }

    wp::ConfigManager cm;
    REQUIRE(cm.load(cfgPath.string()));

    SUBCASE("set_current_video writes video and clears shader") {
        REQUIRE(cm.set_current_video("wallpapers/new.mp4"));

        wp::ConfigManager cm2;
        REQUIRE(cm2.load(cfgPath.string()));
        CHECK(std::filesystem::path(cm2.get_current().video_path).filename() == "new.mp4");
        CHECK(cm2.get_current().shader_path.empty());
    }

    SUBCASE("set_current_shader writes shader and clears video") {
        REQUIRE(cm.set_current_shader("shaders/new.glsl"));

        wp::ConfigManager cm2;
        REQUIRE(cm2.load(cfgPath.string()));
        CHECK(std::filesystem::path(cm2.get_current().shader_path).filename() == "new.glsl");
        CHECK(cm2.get_current().video_path.empty());
    }
}

