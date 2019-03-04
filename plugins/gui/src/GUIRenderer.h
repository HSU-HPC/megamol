/*
 * GUIRenderer.h
 *
 * Copyright (C) 2006 - 2008 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */

/**
 * TODO
 *
 * - Fix drawing order/depth handling of bbox (currently front of bbox is drawn on top of everything)
 * - Fix x and y transformation by View2D class (will be fixed when new CallRender is available)
 *
 */

/**
 * USED HOKEYS:
 *
 * - Show/hide Windows: F12 - F10
 * - Quit program:      Esc, Alt+F4
 *
 */

#ifndef MEGAMOL_GUI_GUIRENDERER_H_INCLUDED
#define MEGAMOL_GUI_GUIRENDERER_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#    pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */
#if defined(_WIN32) && defined(_MANAGED)
#    pragma managed(push, off)
#endif /* defined(_WIN32) && defined(_MANAGED) */


#include "mmcore/CallerSlot.h"
#include "mmcore/CoreInstance.h"
#include "mmcore/Module.h"
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
#include "mmcore/param/Vector2fParam.h"
#include "mmcore/param/Vector3fParam.h"
#include "mmcore/param/Vector4fParam.h"
#include "mmcore/utility/ResourceWrapper.h"
#include "mmcore/view/Input.h"
#include "mmcore/view/Renderer2DModule.h"
#include "mmcore/view/Renderer3DModule.h"

#include "vislib/UTF8Encoder.h"

#include <iomanip> // setprecision
#include <sstream> // stringstream

#ifdef _WIN32
#    include <filesystem> // directory_iterator
#    if _HAS_CXX17
namespace ns_fs = std::filesystem;
#    else
namespace ns_fs = std::experimental::filesystem;
#    endif
#endif

#include <imgui.h>
#include "imgui_impl_opengl3.h"


namespace megamol {
namespace gui {

template <class M, class C> class GUIRenderer : public M {
public:
    /**
     * Answer the name of this module.
     *
     * @return The name of this module.
     */
    static const char* ClassName(void);

    /**
     * Answer a human readable description of this module.
     *
     * @return A human readable description of this module.
     */
    static inline const char* Description(void) { return "Graphical user interface renderer"; }

    /**
     * Answers whether this module is available on the current system.
     *
     * @return 'true' if the module is available, 'false' otherwise.
     */
    static inline bool IsAvailable(void) { return true; }

    /**
     * Initialises a new instance.
     */
    GUIRenderer();

    /**
     * Finalises an instance.
     */
    virtual ~GUIRenderer();

protected:
    virtual bool create() override;

    virtual void release() override;

    virtual bool OnKey(core::view::Key key, core::view::KeyAction action, core::view::Modifiers mods) override;

    virtual bool OnChar(unsigned int codePoint) override;

    virtual bool OnMouseButton(
        core::view::MouseButton button, core::view::MouseButtonAction action, core::view::Modifiers mods) override;

    virtual bool OnMouseMove(double x, double y) override;

    virtual bool OnMouseScroll(double dx, double dy) override;

    virtual bool GetExtents(C& call) override;

    virtual bool Render(C& call) override;


private:
    // TYPES, ENUMS -----------------------------------------------------------

    // Type for
    typedef void (GUIRenderer<M, C>::*GuiFunc)(void);

    /** Type for holding window configuration. */
    typedef struct _gui_window {
        std::string label;          // window label
        bool show;                  // open/close window
        core::view::KeyCode hotkey; // hotkey for opening/closing window
        ImGuiWindowFlags flags;     // imgui window flags
        GuiFunc func;               // pointer to function drawing window content
    } GUIWindow;

    // (Arbitrary) ImGui key map assignment for text manipulation hotkeys (< 512)
    enum TextModHotkeys { CTRL_A = 506, CTRL_C = 507, CTRL_V = 508, CTRL_X = 509, CTRL_Y = 510, CTRL_Z = 511 };

    // VARIABLES --------------------------------------------------------------

    /** The decorated renderer caller slot */
    core::CallerSlot decorated_renderer_slot;

    // ----- Main Parameter Window -----
    /** Spacing of parameter name and hotkey. */
    float hotkey_spacing;
    /** Show/hide only hotkey parameter. */
    bool show_hotkey_params;

    // ----- FPS window -----
    /** Show/hide fps/ms options. */
    bool show_fps_ms_options;
    /** Current time delay since last time fps have been updated. */
    float current_delay;
    /** Maximum delay when fps/ms value should be renewed. */
    float max_delay;
    /** Array holding last fps values. */
    std::vector<float> fps_values;
    std::vector<float> ms_values;
    /** Maximum value in fps_values. */
    float fps_value_scale;
    float ms_value_scale;
    /** Toggle display fps or ms.  */
    int fps_ms_mode;
    /** Maximum count of values in value array. */
    size_t max_value_count;

    /** Float precision for parameter format. */
    int float_print_prec;

    /** Current tooltip hover time. */
    float tooltip_time;
    /** Current hovered tooltip item. */
    ImGuiID tooltip_id;

    /** Array holding the window states. */
    std::list<GUIWindow> windows;

    // FUNCTIONS --------------------------------------------------------------

    /**
     * Callback for drawing the parameter window.
     */
    void drawMainWindowCallback(void);

    /**
     * Draws fps overlay window.
     */
    void drawFpsWindowCallback(void);

    /**
     * Callback for drawing font selection window.
     */
    void drawFontSelectionWindowCallback(void);

    // ---------------------------------

    /**
     * Draws the menu bar.
     */
    void drawWindow(GUIWindow& win);

    /**
     * Draws the menu bar.
     */
    void drawMenu(void);

    /**
     * Draws a parameter for the main parameter window.
     */
    void drawParameter(const core::Module& mod, core::param::ParamSlot& slot);

    /**
     * Draws only button parameters and their hotkeys for the parameter window.
     */
    void drawHotkeyParameter(const core::Module& mod, core::param::ParamSlot& slot);

    /**
     * Show tooltip on hover.
     */
    void hoverToolTip(std::string text, ImGuiID id = 0, float time_start = 0.0f, float time_end = 4.0f);

    /**
     * Show help marker.
     */
    void helpMarkerToolTip(std::string text, std::string label = "(?)");

    /**
     * Update fps and ms value arrays.
     */
    void updateFps(void);

    /**
     * Shutdown megmol core.
     */
    void shutdown(void);

    // ------------------------------------------------------------------------
};


typedef GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D> GUIRenderer2D;
typedef GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D> GUIRenderer3D;


/**
 * GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::GUIRenderer
 */
template <>
inline GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::GUIRenderer()
    : decorated_renderer_slot("decoratedRenderer", "Connects to another 2D Renderer being decorated")
    , hotkey_spacing(0.0f)
    , show_hotkey_params(false)
    , show_fps_ms_options(false)
    , current_delay(0.0f)
    , max_delay(2.0f) // update fps/ms every X second(s)
    , fps_values()
    , ms_values()
    , fps_value_scale(0.0f)
    , ms_value_scale(0.0f)
    , fps_ms_mode(0)
    , max_value_count(50) // max count of stored fps/ms values
    , float_print_prec(3) // float format
    , tooltip_time(0.0f)
    , tooltip_id(0)
    , windows() {

    this->decorated_renderer_slot.SetCompatibleCall<core::view::CallRender2DDescription>();
    this->MakeSlotAvailable(&this->decorated_renderer_slot);
}


/**
 * GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::GUIRenderer
 */
template <>
inline GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::GUIRenderer()
    : decorated_renderer_slot("decoratedRenderer", "Connects to another 3D Renderer being decorated")
    , hotkey_spacing(0.0f)
    , show_hotkey_params(false)
    , show_fps_ms_options(false)
    , current_delay(0.0f)
    , max_delay(2.0f) // update fps/ms every X second(s)
    , fps_values()
    , ms_values()
    , fps_value_scale(0.0f)
    , ms_value_scale(0.0f)
    , fps_ms_mode(0)
    , max_value_count(50) // max count of stored fps/ms values
    , float_print_prec(3) // float format
    , tooltip_time(0.0f)
    , tooltip_id(0)
    , windows() {

    this->decorated_renderer_slot.SetCompatibleCall<core::view::CallRender3DDescription>();
    this->MakeSlotAvailable(&this->decorated_renderer_slot);
}


/**
 * GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::ClassName
 */
template <> inline const char* GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::ClassName(void) {

    return "GUIRenderer2D";
}


/**
 * GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::ClassName
 */
template <> inline const char* GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::ClassName(void) {

    return "GUIRenderer3D";
}


/**
 * GUIRenderer<M, C>::~GUIRenderer
 */
template <class M, class C> GUIRenderer<M, C>::~GUIRenderer() { this->Release(); }


/**
 * GUIRenderer<M, C>::create
 */
template <class M, class C> bool GUIRenderer<M, C>::create() {

    // Window configurations
    this->windows.clear();
    GUIWindow tmp_win;
    // Main Window ------------------------------------------------------------
    tmp_win.label = "MegaMol";
    tmp_win.show = true;
    tmp_win.hotkey = core::view::KeyCode(core::view::Key::KEY_F12);
    tmp_win.flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar; /// | ImGuiWindowFlags_AlwaysAutoResize;
    tmp_win.func = &GUIRenderer<M, C>::drawMainWindowCallback;
    this->windows.push_back(tmp_win);

    // FPS overlay Window -----------------------------------------------------
    tmp_win.label = "fps/ms";
    tmp_win.show = false;
    tmp_win.hotkey = core::view::KeyCode(core::view::Key::KEY_F11);
    tmp_win.flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
    tmp_win.func = &GUIRenderer<M, C>::drawFpsWindowCallback;
    this->windows.push_back(tmp_win);

    // Font Selection Window --------------------------------------------------
    tmp_win.label = "Fonts";
    tmp_win.show = false;
    tmp_win.hotkey = core::view::KeyCode(core::view::Key::KEY_F10);
    tmp_win.flags = ImGuiWindowFlags_AlwaysAutoResize;
    tmp_win.func = &GUIRenderer<M, C>::drawFontSelectionWindowCallback;
    this->windows.push_back(tmp_win);


    // Create ImGui context ---------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // IO settings ------------------------------------------------------------
    ImGuiIO& io = ImGui::GetIO();
    io.IniSavingRate = 5.0f; //  in seconds
    io.IniFilename = "imgui.ini";
    io.LogFilename = "imgui_log.txt";

#ifdef _WIN32
    // Loading additional fonts
    io.FontAllowUserScaling = true;
    io.Fonts->AddFontDefault();
    float font_size = 15.0f;
    std::string ext = ".ttf";
    ImFontConfig config;
    config.OversampleH = 4;
    config.OversampleV = 1;

    const vislib::Array<vislib::StringW>& searchPaths = this->GetCoreInstance()->Configuration().ResourceDirectories();
    for (int i = 0; i < searchPaths.Count(); ++i) {
        for (auto& entry : ns_fs::recursive_directory_iterator(searchPaths[i].PeekBuffer())) {
            if (entry.path().extension().generic_string() == ext) {
                std::string file_path = entry.path().generic_string();
                std::string file_name = entry.path().filename().generic_string();
                if (file_name == "Proggy_Tiny.ttf") {
                    font_size = 10.0f;
                } else if (file_name == "Roboto_Regular.ttf") {
                    font_size = 17.0f;
                } else if (file_name == "Ubuntu_Mono_Regular.ttf") {
                    font_size = 15.0f;
                }
                io.Fonts->AddFontFromFileTTF(file_path.c_str(), font_size, &config);
            }
        }
    }
#endif

    // ImGui Key Map
    io.KeyMap[ImGuiKey_Tab] = static_cast<int>(core::view::Key::KEY_TAB);
    io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(core::view::Key::KEY_LEFT);
    io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(core::view::Key::KEY_RIGHT);
    io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(core::view::Key::KEY_UP);
    io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(core::view::Key::KEY_DOWN);
    io.KeyMap[ImGuiKey_PageUp] = static_cast<int>(core::view::Key::KEY_PAGE_UP);
    io.KeyMap[ImGuiKey_PageDown] = static_cast<int>(core::view::Key::KEY_PAGE_DOWN);
    io.KeyMap[ImGuiKey_Home] = static_cast<int>(core::view::Key::KEY_HOME);
    io.KeyMap[ImGuiKey_End] = static_cast<int>(core::view::Key::KEY_END);
    io.KeyMap[ImGuiKey_Insert] = static_cast<int>(core::view::Key::KEY_INSERT);
    io.KeyMap[ImGuiKey_Delete] = static_cast<int>(core::view::Key::KEY_DELETE);
    io.KeyMap[ImGuiKey_Backspace] = static_cast<int>(core::view::Key::KEY_BACKSPACE);
    io.KeyMap[ImGuiKey_Space] = static_cast<int>(core::view::Key::KEY_SPACE);
    io.KeyMap[ImGuiKey_Enter] = static_cast<int>(core::view::Key::KEY_ENTER);
    io.KeyMap[ImGuiKey_Escape] = static_cast<int>(core::view::Key::KEY_ESCAPE);
    io.KeyMap[ImGuiKey_A] = static_cast<int>(TextModHotkeys::CTRL_A);
    io.KeyMap[ImGuiKey_C] = static_cast<int>(TextModHotkeys::CTRL_C);
    io.KeyMap[ImGuiKey_V] = static_cast<int>(TextModHotkeys::CTRL_V);
    io.KeyMap[ImGuiKey_X] = static_cast<int>(TextModHotkeys::CTRL_X);
    io.KeyMap[ImGuiKey_Y] = static_cast<int>(TextModHotkeys::CTRL_Y);
    io.KeyMap[ImGuiKey_Z] = static_cast<int>(TextModHotkeys::CTRL_Z);

    // Style settings ---------------------------------------------------------
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 3.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.AntiAliasedLines = true;
    style.AntiAliasedFill = true;
    style.DisplaySafeAreaPadding = ImVec2(5.0f, 5.0f);

    // Global settings ---------------------------------------------------------
    ImGui::SetColorEditOptions(ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_RGB |
                               ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar);


    // Init OpenGL for ImGui --------------------------------------------------
    const char* glsl_version = "#version 130"; /// "#version 150"
    ImGui_ImplOpenGL3_Init(glsl_version);

    return true;
}


/**
 * GUIRenderer<M, C>::release
 */
template <class M, class C> void GUIRenderer<M, C>::release() {

    this->windows.clear();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}


/**
 * GUIRenderer<M, C>::OnKey
 */
template <class M, class C>
bool GUIRenderer<M, C>::OnKey(core::view::Key key, core::view::KeyAction action, core::view::Modifiers mods) {

    ImGuiIO& io = ImGui::GetIO();
    auto keyIndex = static_cast<size_t>(key);
    switch (action) {
    case core::view::KeyAction::PRESS:
        io.KeysDown[keyIndex] = true;
        break;
    case core::view::KeyAction::RELEASE:
        io.KeysDown[keyIndex] = false;
        break;
    default:
        break;
    }
    io.KeyCtrl = mods.test(core::view::Modifier::CTRL);
    io.KeyShift = mods.test(core::view::Modifier::SHIFT);
    io.KeyAlt = mods.test(core::view::Modifier::ALT);

    // Check for additional text modification hotkeys
    if (action == core::view::KeyAction::RELEASE) {
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_A)] = false;
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_C)] = false;
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_V)] = false;
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_X)] = false;
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_Y)] = false;
        io.KeysDown[static_cast<size_t>(TextModHotkeys::CTRL_Z)] = false;
    }
    bool hotkeyPressed = true;
    if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_A))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_A);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_C))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_C);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_V))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_V);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_X))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_X);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_Y))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_Y);
    } else if (io.KeyCtrl && ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_Z))) {
        keyIndex = static_cast<size_t>(TextModHotkeys::CTRL_Z);
    } else {
        hotkeyPressed = false;
    }
    if (hotkeyPressed && (action == core::view::KeyAction::PRESS)) {
        io.KeysDown[keyIndex] = true;
    }

    // Exit megamol
    bool exit = (ImGui::IsKeyDown(io.KeyMap[ImGuiKey_Escape])) ||                               // Escape
                ((io.KeyAlt) && (ImGui::IsKeyDown(static_cast<int>(core::view::Key::KEY_F4)))); // Alt + F4
    if (exit) {
        this->shutdown();
        return true;
    }

    // Hotkeys of window(s)
    for (auto& win : this->windows) {
        hotkeyPressed = (ImGui::IsKeyDown(static_cast<int>(win.hotkey.key))) &&
                        (win.hotkey.mods.test(core::view::Modifier::ALT) == io.KeyAlt) &&
                        (win.hotkey.mods.test(core::view::Modifier::CTRL) == io.KeyCtrl) &&
                        (win.hotkey.mods.test(core::view::Modifier::SHIFT) == io.KeyShift);
        if (hotkeyPressed) {
            win.show = !win.show;
        }
    }

    // Check for pressed arameter hotkeys
    hotkeyPressed = false;
    this->GetCoreInstance()->EnumParameters([&, this](const auto& mod, auto& slot) {
        auto param = slot.Parameter();
        if (!param.IsNull()) {
            if (auto* p = slot.template Param<core::param::ButtonParam>()) {
                auto keyCode = p->GetKeyCode();
                hotkeyPressed = (ImGui::IsKeyDown(static_cast<int>(keyCode.key))) &&
                                (keyCode.mods.test(core::view::Modifier::ALT) == io.KeyAlt) &&
                                (keyCode.mods.test(core::view::Modifier::CTRL) == io.KeyCtrl) &&
                                (keyCode.mods.test(core::view::Modifier::SHIFT) == io.KeyShift);
                if (hotkeyPressed) {
                    p->setDirty();
                }
            }
        }
    });
    if (hotkeyPressed) return true;


    auto* cr = this->decorated_renderer_slot.template CallAs<C>();
    if (cr == nullptr) return false;

    core::view::InputEvent evt;
    evt.tag = core::view::InputEvent::Tag::Key;
    evt.keyData.key = key;
    evt.keyData.action = action;
    evt.keyData.mods = mods;
    cr->SetInputEvent(evt);
    if (!(*cr)(core::view::InputCall::FnOnKey)) return false;

    return false;
}


/**
 * GUIRenderer<M, C>::OnChar
 */
template <class M, class C> bool GUIRenderer<M, C>::OnChar(unsigned int codePoint) {

    ImGuiIO& io = ImGui::GetIO();
    io.ClearInputCharacters();
    if (codePoint > 0 && codePoint < 0x10000) io.AddInputCharacter((unsigned short)codePoint);

    auto* cr = this->decorated_renderer_slot.template CallAs<C>();
    if (cr) {
        core::view::InputEvent evt;
        evt.tag = core::view::InputEvent::Tag::Char;
        evt.charData.codePoint = codePoint;
        cr->SetInputEvent(evt);
        if ((*cr)(core::view::InputCall::FnOnChar)) return true;
    }

    return true;
}


/**
 * GUIRenderer<M, C>::OnMouseMove
 */
template <class M, class C> bool GUIRenderer<M, C>::OnMouseMove(double x, double y) {

    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)x, (float)y);

    auto hoverFlags =
        ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_AllowWhenBlockedByPopup;
    if (!ImGui::IsWindowHovered(hoverFlags)) {
        auto* cr = this->decorated_renderer_slot.template CallAs<C>();
        if (cr == nullptr) return false;

        core::view::InputEvent evt;
        evt.tag = core::view::InputEvent::Tag::MouseMove;
        evt.mouseMoveData.x = x;
        evt.mouseMoveData.y = y;
        cr->SetInputEvent(evt);
        if (!(*cr)(core::view::InputCall::FnOnMouseMove)) return false;
    }

    return true;
}


/**
 * GUIRenderer<M, C>::OnMouseButton
 */
template <class M, class C>
bool GUIRenderer<M, C>::OnMouseButton(
    core::view::MouseButton button, core::view::MouseButtonAction action, core::view::Modifiers mods) {

    bool down = (action == core::view::MouseButtonAction::PRESS);
    auto buttonIndex = static_cast<size_t>(button);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[buttonIndex] = down;

    auto hoverFlags =
        ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_AllowWhenBlockedByPopup;
    if (!ImGui::IsWindowHovered(hoverFlags)) {
        auto* cr = this->decorated_renderer_slot.template CallAs<C>();
        if (cr == nullptr) return false;

        core::view::InputEvent evt;
        evt.tag = core::view::InputEvent::Tag::MouseButton;
        evt.mouseButtonData.button = button;
        evt.mouseButtonData.action = action;
        evt.mouseButtonData.mods = mods;
        cr->SetInputEvent(evt);
        if (!(*cr)(core::view::InputCall::FnOnMouseButton)) return false;
    }

    return true;
}


/**
 * GUIRenderer<M, C>::OnMouseScroll
 */
template <class M, class C> bool GUIRenderer<M, C>::OnMouseScroll(double dx, double dy) {

    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)dx;
    io.MouseWheel += (float)dy;

    auto hoverFlags =
        ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_AllowWhenBlockedByPopup;
    if (!ImGui::IsWindowHovered(hoverFlags)) {
        auto* cr = this->decorated_renderer_slot.template CallAs<C>();
        if (cr == nullptr) return false;

        core::view::InputEvent evt;
        evt.tag = core::view::InputEvent::Tag::MouseScroll;
        evt.mouseScrollData.dx = dx;
        evt.mouseScrollData.dy = dy;
        cr->SetInputEvent(evt);
        if (!(*cr)(core::view::InputCall::FnOnMouseScroll)) return false;
    }

    return true;
}


/**
 * GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::GetExtents
 */
template <>
inline bool GUIRenderer<core::view::Renderer2DModule, core::view::CallRender2D>::GetExtents(
    core::view::CallRender2D& call) {

    auto* cr = this->decorated_renderer_slot.CallAs<core::view::CallRender2D>();
    if (cr != nullptr) {
        (*cr) = call;
        if ((*cr)(core::view::AbstractCallRender::FnGetExtents)) {
            call = (*cr);
        }
    } else {
        call.SetBoundingBox(vislib::math::Rectangle<float>(0, 1, 1, 0));
    }

    return true;
}


/**
 * GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::GetExtents
 */
template <>
inline bool GUIRenderer<core::view::Renderer3DModule, core::view::CallRender3D>::GetExtents(
    core::view::CallRender3D& call) {

    auto* cr = this->decorated_renderer_slot.CallAs<core::view::CallRender3D>();
    if (cr != nullptr) {
        (*cr) = call;
        if ((*cr)(core::view::AbstractCallRender::FnGetExtents)) {
            call = (*cr);
        }
    } else {
        call.AccessBoundingBoxes().Clear();
        call.AccessBoundingBoxes().SetWorldSpaceBBox(
            vislib::math::Cuboid<float>(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f));
    }

    return true;
}


/**
 * GUIRenderer<M, C>::Render
 */
template <class M, class C> bool GUIRenderer<M, C>::Render(C& call) {

    auto* cr = this->decorated_renderer_slot.template CallAs<C>();
    if (cr != nullptr) {
        (*cr) = call;
        if ((*cr)(core::view::AbstractCallRender::FnRender)) {
            call = (*cr);
        }
    }

    auto viewportWidth = call.GetViewport().Width();
    auto viewportHeight = call.GetViewport().Height();

    // Set IO stuff
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)viewportWidth, (float)viewportHeight);
    io.DisplayFramebufferScale = ImVec2(1.0, 1.0);
    io.DeltaTime = static_cast<float>(call.LastFrameTime() / 1000.0); // given in milliseconds

    // Start the frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // Construct frame
    for (auto& win : this->windows) {
        this->drawWindow(win);
    }

    // Render the frame
    glViewport(0, 0, viewportWidth, viewportHeight);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update current fps and ms data each frame
    this->updateFps();

    return true;
}


/**
 * GUIRenderer<M, C>::drawMainWindowCallback
 */
template <class M, class C> void GUIRenderer<M, C>::drawMainWindowCallback(void) {

    // Menu -------------------------------------------------------------------
    /// Requires window flag ImGuiWindowFlags_MenuBar
    if (ImGui::BeginMenuBar()) {
        this->drawMenu();
        ImGui::EndMenuBar();
    }

    // Parameters -------------------------------------------------------------
    ImGui::Text("PARAMETERS");
    std::string color_param_help = "Hover parameter for description tooltip";
    this->helpMarkerToolTip(color_param_help);


    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() * 0.5f); // set general proportional item width

    int overrideState = -1;
    if (ImGui::Button("Expand All")) {
        overrideState = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Collapse All")) {
        overrideState = 0;
    }
    ImGui::SameLine();
    ImGui::Checkbox("Show Hotkeys", &this->show_hotkey_params);
    ImGui::Separator();

    const core::Module* currentMod = nullptr;
    bool currentModOpen = false;
    this->GetCoreInstance()->EnumParameters([&, this](const auto& mod, auto& slot) {
        if (currentMod != &mod) {
            currentMod = &mod;

            auto headerId = ImGui::GetID(mod.FullName());
            auto headerState = overrideState;
            if (headerState == -1) {
                headerState = ImGui::GetStateStorage()->GetInt(headerId, 0); // 0=close 1=open
            }
            ImGui::GetStateStorage()->SetInt(headerId, headerState);
            currentModOpen = ImGui::CollapsingHeader(mod.FullName());
        }
        if (currentModOpen) {
            if (this->show_hotkey_params) {
                this->drawHotkeyParameter(mod, slot);
            } else {
                this->drawParameter(mod, slot);
            }
        }
    });

    ImGui::PopItemWidth();
}


/**
 * GUIRenderer<M, C>::drawFpsWindowCallback
 */
template <class M, class C> void GUIRenderer<M, C>::drawFpsWindowCallback(void) {

    if (ImGui::RadioButton("fps", (this->fps_ms_mode == 0))) {
        this->fps_ms_mode = 0;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("ms", (this->fps_ms_mode == 1))) {
        this->fps_ms_mode = 1;
    }
    ImGui::SameLine(140.0f);
    ImGui::Checkbox("Options", &this->show_fps_ms_options);

    // Default for this->fps_ms_mode == 0
    std::string label = "###fps_ms"; /// use hidden label
    std::vector<float>* arr = &this->fps_values;
    float val_scale = this->fps_value_scale;
    if (this->fps_ms_mode == 1) {
        arr = &this->ms_values;
        val_scale = this->ms_value_scale;
    }
    float* data = arr->data();
    int count = (int)arr->size();

    std::string val;
    if (!arr->empty()) {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(this->float_print_prec); //<< std::setw(7)
        stream << arr->back();
        val = stream.str();
    }
    ImGui::PlotHistogram(label.c_str(), data, count, 0, val.c_str(), 0.0f, val_scale, ImVec2(0, 50));

    if (this->show_fps_ms_options) {

        std::stringstream float_stream;
        float_stream << "%." << this->float_print_prec << "f";
        std::string float_format = float_stream.str();
        if (ImGui::InputFloat("Refresh Rate", &this->max_delay, 0.0f, 0.0f, float_format.c_str(),
                ImGuiInputTextFlags_EnterReturnsTrue)) {
            this->fps_values.clear();
            this->ms_values.clear();
        }
        std::string help = "Changes clear all values";
        this->helpMarkerToolTip(help);

        int mvc = (int)this->max_value_count;
        ImGui::InputInt("Stored Values Count", &mvc, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue);

        if (ImGui::Button("Current Value")) {
            ImGui::SetClipboardText(val.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("All Values")) {
            std::stringstream stream;
            stream << std::fixed << std::setprecision(this->float_print_prec); //<< std::setw(7)

            for (std::vector<float>::reverse_iterator i = (*arr).rbegin(); i != (*arr).rend(); ++i) {
                stream << (*i) << "\n";
            }

            ImGui::SetClipboardText(stream.str().c_str());
        }
        ImGui::SameLine();
        ImGui::Text("Copy to Clipborad");
        help = "Values are copied in chronological order (newest first)";
        this->helpMarkerToolTip(help);
    }
}


/**
 * GUIRenderer<M, C>::drawFontSelectionWindowCallback
 */
template <class M, class C> void GUIRenderer<M, C>::drawFontSelectionWindowCallback(void) {

    ImGuiIO& io = ImGui::GetIO();

    ImFont* font_current = ImGui::GetFont();

    if (ImGui::BeginCombo("Select Font", font_current->GetDebugName())) {
        for (int n = 0; n < io.Fonts->Fonts.Size; n++) {
            if (ImGui::Selectable(io.Fonts->Fonts[n]->GetDebugName(), (io.Fonts->Fonts[n] == font_current)))
                io.FontDefault = io.Fonts->Fonts[n];
        }
        ImGui::EndCombo();
    }
}


/**
 * GUIRenderer<M, C>::drawWindow
 */
template <class M, class C> void GUIRenderer<M, C>::drawWindow(GUIWindow& win) {

    if (win.show) {

        ImGui::SetNextWindowSize(ImVec2(400.0f, 300.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(5.0f, 5.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(1.0f);

        if (!ImGui::Begin(win.label.c_str(), &win.show, win.flags)) {
            ImGui::End(); // early ending
            return;
        }

        (this->*win.func)();

        ImGui::End();
    }
}


/**
 * GUIRenderer<M, C>::drawMenu
 */
template <class M, class C> void GUIRenderer<M, C>::drawMenu(void) {

    // File
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit", "'Esc', ALT + 'F4'")) {
            this->shutdown();
        }
        ImGui::EndMenu();
    }

    // Windows
    if (ImGui::BeginMenu("Windows")) {
        for (auto& win : this->windows) {
            bool win_open = win.show;
            if (ImGui::MenuItem(win.label.c_str(), win.hotkey.ToString().c_str(), &win_open)) {
                win.show = !win.show;
            }
        }
        ImGui::EndMenu();
    }

    // Help
    bool open_popup = false;
    if (ImGui::BeginMenu("Help")) {
        const std::string gitLink = "https://github.com/UniStuttgart-VISUS/megamol";
        const std::string mmLink = "https://megamol.org/";
        const std::string helpLink = "https://github.com/UniStuttgart-VISUS/megamol/blob/master/Readme.md";
        const std::string hint = "Copy Link to Clipboard";
        if (ImGui::MenuItem("GitHub")) {
            ImGui::SetClipboardText(gitLink.c_str());
        }
        this->hoverToolTip(hint);
        if (ImGui::MenuItem("Readme")) {
            ImGui::SetClipboardText(helpLink.c_str());
        }
        this->hoverToolTip(hint);
        if (ImGui::MenuItem("Web Page")) {
            ImGui::SetClipboardText(mmLink.c_str());
        }
        this->hoverToolTip(hint);
        ImGui::Separator();
        if (ImGui::MenuItem("About...")) {
            open_popup = true;
        }
        ImGui::EndMenu();
    }

    // PopUp
    std::stringstream about_stream;
    about_stream << "MegaMol is GREAT!" << std::endl << "Using Dear ImGui " << IMGUI_VERSION << std::endl;
    std::string about = about_stream.str();
    if (open_popup) {
        ImGui::OpenPopup("About");
    }
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(about.c_str());
        ImGui::Separator();
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }
}


/**
 * GUIRenderer<M, C>::drawParameter
 */
template <class M, class C>
void GUIRenderer<M, C>::drawParameter(const core::Module& mod, core::param::ParamSlot& slot) {

    std::string help;

    auto param = slot.Parameter();
    if (!param.IsNull()) {
        std::string label = slot.Name().PeekBuffer();
        std::string desc = slot.Description().PeekBuffer();

        std::stringstream float_stream;
        float_stream << "%." << this->float_print_prec << "f";
        std::string float_format = float_stream.str();

        if (auto* p = slot.template Param<core::param::BoolParam>()) {
            auto value = p->Value();
            if (ImGui::Checkbox(label.c_str(), &value)) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.template Param<core::param::ButtonParam>()) {
            std::string hotkeyLabel(label.c_str());
            hotkeyLabel += " (";
            hotkeyLabel += p->GetKeyCode().ToString();
            hotkeyLabel += ")";

            if (ImGui::Button(hotkeyLabel.c_str())) {
                p->setDirty();
            }
        } else if (auto* p = slot.template Param<core::param::ColorParam>()) {
            core::param::ColorParam::ColorType value = p->Value();
            auto color_flags = ImGuiColorEditFlags_AlphaPreview; // | ImGuiColorEditFlags_Float;
            if (ImGui::ColorEdit4(label.c_str(), (float*)value.data(), color_flags)) {
                p->SetValue(value);
            }

            help = "[Click] on the colored square to open a color picker.\n"
                   "[CTRL+Click] on individual component to input value.\n"
                   "[Right-Click] on the individual color widget to show options.";
        } else if (auto* p = slot.template Param<core::param::EnumParam>()) {
            // XXX: no UTF8 fanciness required here?
            auto map = p->getMap();
            auto key = p->Value();
            if (ImGui::BeginCombo(label.c_str(), map[key].PeekBuffer())) {
                auto iter = map.GetConstIterator();
                while (iter.HasNext()) {
                    auto pair = iter.Next();
                    bool isSelected = (pair.Key() == key);
                    if (ImGui::Selectable(pair.Value().PeekBuffer(), isSelected)) {
                        p->SetValue(pair.Key());
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        } else if (auto* p = slot.template Param<core::param::FlexEnumParam>()) {
            // XXX: no UTF8 fanciness required here?
            auto value = p->Value();
            if (ImGui::BeginCombo(label.c_str(), value.c_str())) {
                for (auto valueOption : p->getStorage()) {
                    bool isSelected = (valueOption == value);
                    if (ImGui::Selectable(valueOption.c_str(), isSelected)) {
                        p->SetValue(valueOption);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        } else if (auto* p = slot.template Param<core::param::FloatParam>()) {
            auto value = p->Value();
            if (ImGui::InputFloat(
                    label.c_str(), &value, 0.0f, 0.0f, float_format.c_str(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.template Param<core::param::IntParam>()) {
            auto value = p->Value();
            if (ImGui::InputInt(label.c_str(), &value, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue)) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.template Param<core::param::Vector2fParam>()) {
            auto value = p->Value();
            if (ImGui::InputFloat2(label.c_str(), value.PeekComponents(), float_format.c_str(),
                    ImGuiInputTextFlags_EnterReturnsTrue)) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.template Param<core::param::Vector3fParam>()) {
            auto value = p->Value();
            if (ImGui::InputFloat3(label.c_str(), value.PeekComponents(), float_format.c_str(),
                    ImGuiInputTextFlags_EnterReturnsTrue)) {
                p->SetValue(value);
            }
        } else if (auto* p = slot.template Param<core::param::Vector4fParam>()) {
            auto value = p->Value();
            if (ImGui::InputFloat4(label.c_str(), value.PeekComponents(), float_format.c_str(),
                    ImGuiInputTextFlags_EnterReturnsTrue)) {
                p->SetValue(value);
            }
        } else { // if (auto* p = slot.Param<core::param::StringParam>()) {
            // XXX: UTF8 conversion and allocation every frame is horrific inefficient.
            vislib::StringA valueString;
            vislib::UTF8Encoder::Encode(valueString, param->ValueString());

            size_t bufferLength = 2048; /// std::min(4096, (valueString.Length() + 1) * 2);
            char* buffer = new char[bufferLength];
            memcpy(buffer, valueString.PeekBuffer(), valueString.Length() + 1);

            if (ImGui::InputText(label.c_str(), buffer, bufferLength, ImGuiInputTextFlags_EnterReturnsTrue)) {
                vislib::UTF8Encoder::Decode(valueString, vislib::StringA(buffer));
                param->ParseValue(valueString);
            }
            delete[] buffer;

            help = "Press [Return] to confirm changes.";
        }

        this->hoverToolTip(desc, ImGui::GetID(label.c_str()), 0.75f);

        this->helpMarkerToolTip(help);
    }
}


/**
 * GUIRenderer<M, C>::drawHotkeyParameter
 */
template <class M, class C>
void GUIRenderer<M, C>::drawHotkeyParameter(const core::Module& mod, core::param::ParamSlot& slot) {

    auto param = slot.Parameter();
    if (!param.IsNull()) {
        if (auto* p = slot.template Param<core::param::ButtonParam>()) {
            auto label = std::string(slot.Name().PeekBuffer());
            std::string desc = slot.Description().PeekBuffer();
            auto keycode = p->GetKeyCode().ToString();

            ImGui::Text(label.c_str());
            this->hoverToolTip(desc, ImGui::GetID(label.c_str()), 0.75f);

            // Adapt spacing between label and hotkey string
            auto labelLength = ImGui::GetFontSize() * (float)label.length();
            this->hotkey_spacing = (labelLength > this->hotkey_spacing) ? (labelLength) : (this->hotkey_spacing);
            ImGui::SameLine(this->hotkey_spacing);

            ImGui::Text(keycode.c_str());
            this->hoverToolTip(desc, ImGui::GetID(keycode.c_str()), 0.75f);

            ImGui::Separator();
        }
    }
}


/**
 * GUIRenderer<M, C>::updateFps
 */
template <class M, class C> void GUIRenderer<M, C>::updateFps(void) {

    ImGuiIO& io = ImGui::GetIO();

    this->current_delay += io.DeltaTime;
    if (this->current_delay >= (1.0f / this->max_delay)) {

        const float scale_fac = 1.5f;

        size_t size = this->fps_values.size();
        if (size != this->max_value_count) {
            if (size > this->max_value_count) {
                this->fps_values.erase(
                    this->fps_values.begin(), this->fps_values.begin() + (size - this->max_value_count));
            } else if (size < this->max_value_count) {
                this->fps_values.insert(this->fps_values.begin(), (this->max_value_count - size), 0.0f);
            }
        }
        if (size > 0) {
            this->fps_values.erase(this->fps_values.begin());
            this->fps_values.push_back(io.Framerate);
            float value_max = 0.0f;
            for (auto& v : this->fps_values) {
                value_max = (v > value_max) ? (v) : (value_max);
            }
            this->fps_value_scale = value_max * scale_fac;
        }

        size = this->ms_values.size();
        if (size != this->max_value_count) {
            if (size > this->max_value_count) {
                this->ms_values.erase(
                    this->ms_values.begin(), this->ms_values.begin() + (size - this->max_value_count));
            } else if (size < this->max_value_count) {
                this->ms_values.insert(this->ms_values.begin(), (this->max_value_count - size), 0.0f);
            }
        }
        if (size > 0) {
            this->ms_values.erase(this->ms_values.begin());
            this->ms_values.push_back(io.DeltaTime * 1000.0f); // scale to milliseconds
            float value_max = 0.0f;
            for (auto& v : this->ms_values) {
                value_max = (v > value_max) ? (v) : (value_max);
            }
            this->ms_value_scale = value_max * scale_fac;
        }

        this->current_delay = 0.0f;
    }
}


/**
 * GUIRenderer<M, C>::hoverToolTip
 */
template <class M, class C>
void GUIRenderer<M, C>::hoverToolTip(std::string text, ImGuiID id, float time_start, float time_end) {

    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsItemHovered()) {
        bool show_tooltip = false;
        if (time_start > 0.0f) {
            if (this->tooltip_id != id) {
                this->tooltip_time = 0.0f;
                this->tooltip_id = id;
            } else {
                if ((this->tooltip_time > time_start) && (this->tooltip_time < (time_start + time_end))) {
                    show_tooltip = true;
                }
                this->tooltip_time += io.DeltaTime;
            }
        } else {
            show_tooltip = true;
        }

        if (show_tooltip) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    } else {
        if ((time_start > 0.0f) && (this->tooltip_id == id)) {
            this->tooltip_time = 0.0f;
        }
    }
}


/**
 * GUIRenderer<M, C>::helpMarkerToolTip
 */
template <class M, class C> void GUIRenderer<M, C>::helpMarkerToolTip(std::string text, std::string label) {

    if (!text.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled(label.c_str());
        this->hoverToolTip(text);
    }
}


/**
 * GUIRenderer<M, C>::shutdown
 */
template <class M, class C> void GUIRenderer<M, C>::shutdown(void) {

    vislib::sys::Log::DefaultLog.WriteInfo(">>>>> GUIRenderer: Initialising megamol core instance shutdown.");
    this->GetCoreInstance()->Shutdown();
}


} // namespace gui
} // namespace megamol

#endif // MEGAMOL_GUI_GUIRENDERER_H_INCLUDED
