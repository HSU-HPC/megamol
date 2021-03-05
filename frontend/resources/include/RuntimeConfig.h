/*
 * RuntimeConfig.h
 *
 * Copyright (C) 2021 by VISUS (Universitaet Stuttgart).
 * Alle Rechte vorbehalten.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>

namespace megamol {
namespace frontend_resources {

// make sure that all configuration parameters have sane and useful and EXPLICIT initialization values!
struct RuntimeConfig {
    std::string program_invocation_string = "";

    // general stuff
    using Path = std::string;
    using StringPair = std::pair<std::string/*Config*/, std::string/*Value*/>;

    std::vector<Path> configuration_files = {"megamol_config.lua"};               // set only via (multiple) --config in CLI
    std::vector<std::string> configuration_file_contents = {};
    std::vector<StringPair> cli_options_from_configs = {};                        // mmSetCliOption - set config/option values accepted in CLI
    std::vector<std::string> configuration_file_contents_as_cli = {};
    Path application_directory = "";                                              // mmSetAppDir
    std::vector<Path> resource_directories = {};                                  // mmAddResourceDir
    std::vector<Path> shader_directories = {};                                    // mmAddShaderDir
    Path log_file = "megamol_log.txt";                                            // mmSetLogFile
    unsigned int log_level = 200;                                                 // mmSetLogLevel
    unsigned int echo_level = 200;                                                // mmSetEchoLevel
    std::vector<Path> project_files = {};                                         // NEW: mmLoadProject - project files are loaded after services are up
    std::map<std::string/*Key*/, std::string/*Value*/> global_key_values = {};    // mmSetGlobalValue + mmGetGlobalValue

    // detailed and service-specific configurations
    // every CLI option can be set via the config file using mmSetConfigValue
    // e.g. "--window 100x200" => mmSetConfigValue("window", "100x200")
    //      "--fullscreen"     => mmSetConfigValue("fullscreen", "on")
    bool interactive = false;
    std::string lua_host_address = "tcp://127.0.0.1:33333";
    bool lua_host_port_retry = true;
    bool opengl_khr_debug = false;
    bool opengl_vsync = false;
    std::optional<std::pair<unsigned int /*width*/,unsigned int /*height*/>> window_size = std::nullopt; // if not set, GLFW service will open window with 3/4 of monitor resolution 
    std::optional<std::pair<unsigned int /*x*/,unsigned int /*y*/>>          window_position = std::nullopt;
    enum WindowMode {
        fullscreen   = 1 << 0,
        nodecoration = 1 << 1,
        topmost      = 1 << 2,
        nocursor     = 1 << 3,
    };
    unsigned int window_mode = 0;
    unsigned int window_monitor = 0;


    // add or update a key-value pair
    void global_value_insert(std::string const& key, std::string const& value) {
        global_key_values.insert_or_assign(key, value);
    }

    // retrieve value for given key. if key is present, the optional holds the value.
    std::optional<std::string> global_value_get(std::string const& key) const {
        auto value_it = global_key_values.find(key);
        if (value_it != global_key_values.end()) {
            return std::optional{value_it->second};
        }
        else {
            return std::nullopt;
        }
    }

};

} /* end namespace frontend_resources */
} /* end namespace megamol */
