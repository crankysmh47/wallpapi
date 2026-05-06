#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "logger.hpp"

int main(int argc, char** argv) {
    // Tests call production code that logs; initialize logging once.
    wp::Logger::init();

    doctest::Context ctx;
    ctx.applyCommandLine(argc, argv);
    return ctx.run();
}

