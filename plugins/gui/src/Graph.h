/*
 * Graph.h
 *
 * Copyright (C) 2019 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOL_GUI_GRAPH_H_INCLUDED
#define MEGAMOL_GUI_GRAPH_H_INCLUDED

#include "vislib/sys/Log.h"

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"

#include <vector>
#include <map>

#include "mmcore/CoreInstance.h"
#include "mmcore/Module.h"
#include "mmcore/utility/plugins/AbstractPluginInstance.h"
#include "utility/plugins/PluginManager.h"

#include "mmcore/param/BoolParam.h"
#include "mmcore/param/ButtonParam.h"
#include "mmcore/param/ColorParam.h"
#include "mmcore/param/EnumParam.h"
#include "mmcore/param/FilePathParam.h"
#include "mmcore/param/FlexEnumParam.h"
#include "mmcore/param/FloatParam.h"
#include "mmcore/param/IntParam.h"
#include "mmcore/param/ParamSlot.h"
#include "mmcore/param/StringParam.h"
#include "mmcore/param/TernaryParam.h"
#include "mmcore/param/TransferFunctionParam.h"
#include "mmcore/param/Vector2fParam.h"
#include "mmcore/param/Vector3fParam.h"
#include "mmcore/param/Vector4fParam.h"


namespace megamol {
namespace gui {

class Graph {
public:

    // GRAPH DATA STRUCTURE ---------------------------------------------------

    enum ParamType {
        BUTTON,
        BOOL,
        COLOR,
        ENUM,
        FILEPATH,
        FLEXENUM,
        FLOAT,
        INT,
        STRING,
        TERNARY,
        TRANSFERFUNCTION,
        VECTOR2F,
        VECTOR3F,
        VECTOR4F,
        UNKNOWN
    };

    enum CallSlotType { CALLEE, CALLER };

    // Forward declaration
    class Call;
    class Module;

    class ParamSlot {
    public:
        // Initialized on loading
        std::string class_name;
        std::string description;
        Graph::ParamType type;

        // Initilized after/on creation
        std::string full_name;
    };

    class CallSlot {
    public:
        // Initialized on loading 
        std::string name;
        std::string description;
        std::vector<size_t> compatible_call_idxs; // (Storing only indices of compatible calls for faster comparison.)
        std::shared_ptr<Graph::Module> parent_module;
        Graph::CallSlotType type;

        // Initilized after/on creation
        std::vector<std::shared_ptr<Graph::Call>> connected_calls;

        // Functions ------------------

        inline ImVec2 GetCallSlotPos(void) const {
            ImVec2 retpos = ImVec2(-1.0f, -1.0f);
            if (this->parent_module != nullptr) {
                auto slot_count = this->parent_module->call_slots[this->type].size();
                size_t slot_idx = 0;
                for (size_t i = 0; i < slot_count; i++) {
                    if (this->name == this->parent_module->call_slots[this->type][i].name) {
                        slot_idx = i;
                    }
                }
                auto pos = this->parent_module->gui.position;
                auto size = this->parent_module->gui.size;
                retpos = ImVec2(pos.x + ((this->type == Graph::CallSlotType::CALLER)?(size.x):(0.0f)), pos.y + size.y * ((float)slot_idx + 1) / ((float)slot_count + 1));
            }
            return retpos;
        }
    };

    class Call {
    public:
        // Initialized on loading 
        std::string class_name;
        std::string description;
        std::string plugin_name;
        std::vector<std::string> functions;

        // Initilized after/on creation
        std::map<Graph::CallSlotType, std::shared_ptr<Graph::CallSlot>> connected_call_slots; 

        struct Gui {

        } gui;
    };

    class Module {
    public:

        // Initialized on loading 
        std::string class_name;
        std::string description;
        std::string plugin_name;
        std::vector<Graph::ParamSlot> param_slots;
        std::map<Graph::CallSlotType, std::vector<Graph::CallSlot>> call_slots;
        bool is_view;

        // Initilized after/on creation
        std::string name;
        std::string full_name;
        std::string instance;

        struct Gui {
            ImVec2 position;
            ImVec2 size;
        } gui;
    };

    // CLASS ------------------------------------------------------------------

    Graph(void);

    virtual ~Graph(void);

    bool AddModule(const std::string& module_class_name);

    bool AddCall(const std::string& call_class_name);

    bool UpdateAvailableModulesCallsOnce(const megamol::core::CoreInstance* core_instance);

    inline const std::vector<Graph::Module>& GetAvailableModulesList(void) const { return this->modules_list; }

    inline const std::string GetCompatibleCallNames(size_t idx) const {
        if (idx < this->calls_list.size()) {
            return this->calls_list[idx].class_name;
        }
        return std::string();
    }

    inline std::vector<Graph::Module>& GetGraphModules(void) { return this->modules_graph; }

    inline std::vector<Graph::Call>& GetGraphCalls(void) { return this->calls_graph; }

    bool SetSelectedCallSlot(const std::string& module_full_name, const std::string& slot_name);

    std::shared_ptr<Graph::CallSlot>& GetSelectedCallSlot(void) {
        return this->selected_call_slot;
    }

    inline void ResetSelectedCallSlot(void) {
        this->selected_call_slot.reset();
    }

    /**
     * Only used for prototype to be able to store current graph to lua project file.
     * Later use FileUtils->SaveProjectFile provided in GUI menu.
     */
    bool PROTOTYPE_SaveGraph(std::string project_filename, megamol::core::CoreInstance* cor_iInstance);

private:

    // VARIABLES --------------------------------------------------------------

    std::vector<Graph::Module> modules_graph;
    std::vector<Graph::Call> calls_graph;

    std::vector<Graph::Call> calls_list;
    std::vector<Graph::Module> modules_list;

    std::shared_ptr<Graph::CallSlot> selected_call_slot;

    // FUNCTIONS --------------------------------------------------------------

    bool read_call_data(Graph::Call& call, const std::shared_ptr<const megamol::core::factories::CallDescription> call_desc); 
    bool read_module_data(Graph::Module& mod, const std::shared_ptr<const megamol::core::factories::ModuleDescription> mod_desc);
    
    // ------------------------------------------------------------------------
};

} // namespace gui
} // namespace megamol

#endif // MEGAMOL_GUI_GRAPH_H_INCLUDED