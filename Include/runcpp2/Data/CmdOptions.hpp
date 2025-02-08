#ifndef RUNCPP2_DATA_CMD_OPTIONS_HPP
#define RUNCPP2_DATA_CMD_OPTIONS_HPP

namespace runcpp2
{
    enum class CmdOptions
    {
        NONE,
        RESET_CACHE,
        RESET_USER_CONFIG,
        EXECUTABLE,
        HELP,
        RESET_DEPENDENCIES,
        LOCAL,
        SHOW_USER_CONFIG,
        SCRIPT_TEMPLATE,
        WATCH,
        BUILD,
        VERSION,
        LOG_LEVEL,
        CONFIG_FILE,
        CLEANUP,
        BUILD_SOURCE_ONLY,
        THREADS,
        COUNT
    };
}

#endif
