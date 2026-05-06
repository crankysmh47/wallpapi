#include <doctest/doctest.h>

#include "wallpaper_scan.hpp"

#include <filesystem>
#include <fstream>

TEST_CASE("find_first_by_extension selects first lexicographically and supports images") {
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "wallpapi-tests" / "scan";
    fs::create_directories(dir);

    // Clean any previous run.
    for (auto& e : fs::directory_iterator(dir)) {
        std::error_code ec;
        fs::remove(e.path(), ec);
    }

    std::ofstream(dir / "b.mp4", std::ios::binary).close();
    std::ofstream(dir / "a.jpg", std::ios::binary).close();
    std::ofstream(dir / "c.png", std::ios::binary).close();

    auto found = wp::find_first_by_extension(dir, {".mp4", ".jpg", ".png"});
    REQUIRE(found.has_value());
    CHECK(found->filename() == "a.jpg"); // lexicographic
}

TEST_CASE("find_first_by_extension returns empty when none match") {
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "wallpapi-tests" / "scan-empty";
    fs::create_directories(dir);
    std::ofstream(dir / "x.txt", std::ios::binary).close();

    auto found = wp::find_first_by_extension(dir, {".mp4"});
    CHECK(!found.has_value());
}

TEST_CASE("list_files_by_extension returns sorted matches only") {
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "wallpapi-tests" / "scan-list";
    fs::create_directories(dir);

    // Clean any previous run.
    for (auto& e : fs::directory_iterator(dir)) {
        std::error_code ec;
        fs::remove(e.path(), ec);
    }

    std::ofstream(dir / "b.mp4", std::ios::binary).close();
    std::ofstream(dir / "a.jpg", std::ios::binary).close();
    std::ofstream(dir / "z.txt", std::ios::binary).close();

    const auto list = wp::list_files_by_extension(dir, {".mp4", ".jpg"});
    REQUIRE(list.size() == 2);
    CHECK(list[0].filename() == "a.jpg");
    CHECK(list[1].filename() == "b.mp4");
}

