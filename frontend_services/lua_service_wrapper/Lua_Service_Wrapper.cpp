/*
 * Lua_Service_Wrapper.cpp
 *
 * Copyright (C) 2020 by MegaMol Team
 * Alle Rechte vorbehalten.
 */

// TODO: we need this #define because inclusion of LuaHostService.h leads to windows header inclusion errors.
// this stems from linking ZMQ via CMake now being PUBLIC in the core lib. i dont know how to solve this "the right way". 
#define _WINSOCKAPI_
#include "Lua_Service_Wrapper.hpp"


#include "mmcore/utility/LuaHostService.h"

#include "Screenshots.h"

// local logging wrapper for your convenience until central MegaMol logger established
#include "Window_Events.h"
#include "mmcore/utility/log/Log.h"
static void log(const char* text) {
    const std::string msg = "Lua_Service_Wrapper: " + std::string(text) + "\n";
    megamol::core::utility::log::Log::DefaultLog.WriteInfo(msg.c_str());
}
static void log(std::string text) { log(text.c_str()); }

namespace {
    // used to abort a service callback if we are already inside a service wrapper callback
    struct RecursionGuard {
        int& state;

        RecursionGuard(int& s) : state{s} { state++; }
        ~RecursionGuard() { state--; }
        bool abort() { return state > 1; }
    };
}

namespace megamol {
namespace frontend {

Lua_Service_Wrapper::Lua_Service_Wrapper() {
    // init members to default states
}

Lua_Service_Wrapper::~Lua_Service_Wrapper() {
    // clean up raw pointers you allocated with new, which is bad practice and nobody does
}

bool Lua_Service_Wrapper::init(void* configPtr) {
    if (configPtr == nullptr) return false;

    return init(*static_cast<Config*>(configPtr));
}

#define luaAPI (*m_config.lua_api_ptr)
#define m_network_host reinterpret_cast<megamol::core::utility::LuaHostNetworkConnectionsBroker*>(m_network_host_pimpl.get())

bool Lua_Service_Wrapper::init(const Config& config) {
    if (!config.lua_api_ptr) {
        log("failed initialization because LuaAPI is nullptr");
        return false;
    }

    m_config = config;

    this->m_providedResourceReferences = {};

    this->m_requestedResourcesNames = 
    {
        "FrontendResourcesList",
        "GLFrontbufferToPNG_ScreenshotTrigger", // for screenshots
        "WindowEvents" // for LastFrameTime
    }; //= {"ZMQ_Context"};

    m_network_host_pimpl = std::unique_ptr<void, std::function<void(void*)>>(
        new megamol::core::utility::LuaHostNetworkConnectionsBroker{},
        [](void* ptr) { delete reinterpret_cast<megamol::core::utility::LuaHostNetworkConnectionsBroker*>(ptr); }
    );

    m_network_host->broker_address = m_config.host_address;
    bool host_ok = m_network_host->spawn_connection_broker();

    if (host_ok) {
        log("initialized successfully");
    } else {
        log("failed to start lua host");
    }

    return host_ok;
}

void Lua_Service_Wrapper::close() {
    m_config = {}; // default to nullptr

    m_network_host->close();
}

std::vector<ModuleResource>& Lua_Service_Wrapper::getProvidedResources() {
    return m_providedResourceReferences; // empty
}

const std::vector<std::string> Lua_Service_Wrapper::getRequestedResourceNames() const {
    return m_requestedResourcesNames;
}

void Lua_Service_Wrapper::setRequestedResources(std::vector<ModuleResource> resources) {
    // TODO: do something with ZMQ resource we get here
    m_requestedResourceReferences = resources;

    auto& list_callback = resources[0].getResource< 
        std::function< std::vector<std::string> (void)> 
    >();
    luaAPI.setListResourcesCallback(list_callback);

    auto& screenshot_callback = m_requestedResourceReferences[1].getResource< std::function<bool(std::string const&)> >();
    luaAPI.setScreenshotCallback(screenshot_callback);

    luaAPI.setLastFrameTimeCallback([this]() {
        auto window_events = &this->m_requestedResourceReferences[2].getResource<megamol::module_resources::WindowEvents>();
        // BUG: this is NOT the duration of the last frame
        return static_cast<float>(window_events->previous_state.time);
    });
}

// -------- main loop callbacks ---------

#define recursion_guard \
    RecursionGuard rg{m_service_recursion_depth}; \
    if (rg.abort()) return;

void Lua_Service_Wrapper::updateProvidedResources() {
    recursion_guard;
    // we want lua to be the first thing executed in main loop
    // so we do all the lua work here

    bool need_to_shutdown = false; // e.g. mmQuit should set this to true

    // fetch Lua requests from ZMQ queue, execute, and give back result
    if (!m_network_host->request_queue.empty()) {
        auto lua_requests = std::move(m_network_host->get_request_queue());
        std::string result;
        while (!lua_requests.empty()) {
            auto& request = lua_requests.front();

            luaAPI.RunString(request.request, result);
            request.answer_promise.get().set_value(result);

            lua_requests.pop();
            result.clear();
        }
    }

    need_to_shutdown |= luaAPI.getShutdown();

    if (need_to_shutdown)
        this->setShutdown();
}

void Lua_Service_Wrapper::digestChangedRequestedResources() { recursion_guard; }

void Lua_Service_Wrapper::resetProvidedResources() { recursion_guard; }

void Lua_Service_Wrapper::preGraphRender() {
    recursion_guard;

    // this gets called right before the graph is told to render something
    // e.g. you can start a start frame timer here

    // rendering via MegaMol View is called after this function finishes
    // in the end this calls the equivalent of ::mmcRenderView(hView, &renderContext)
    // which leads to view.Render()
}

void Lua_Service_Wrapper::postGraphRender() {
    recursion_guard;

    // the graph finished rendering and you may more stuff here
    // e.g. end frame timer
    // update window name
    // swap buffers, glClear
}


} // namespace frontend
} // namespace megamol
