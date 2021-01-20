/*
 * AbstractOSPRayRenderer.cpp
 * Copyright (C) 2009-2015 by MegaMol Team
 * Alle Rechte vorbehalten.
 */

#include "stdafx.h"
#include "AbstractOSPRayRenderer.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include "mmcore/param/BoolParam.h"
#include "mmcore/param/EnumParam.h"
#include "mmcore/param/FilePathParam.h"
#include "mmcore/param/FloatParam.h"
#include "mmcore/param/IntParam.h"
#include "mmcore/utility/log/Log.h"
#include "mmcore/utility/sys/SystemInformation.h"
#include "vislib/graphics/gl/FramebufferObject.h"
#include "vislib/sys/Path.h"
#include <stdio.h>

namespace megamol {
namespace ospray {

    void ospErrorCallback(OSPError err, const char* details) {
        megamol::core::utility::log::Log::DefaultLog.WriteError("OSPRay Error %u: %s", err, details);
    }

    AbstractOSPRayRenderer::AbstractOSPRayRenderer(void)
            : core::view::Renderer3DModule_2()
            , _accumulateSlot("accumulate", "Activates the accumulation buffer")
            // general renderer parameters
            , _rd_spp("SamplesPerPixel", "Samples per pixel")
            , _rd_maxRecursion("maxRecursion", "Maximum ray recursion depth")
            , _rd_type("Type", "Select between SciVis and PathTracer")
            , _shadows("SciVis::Shadows", "Enables/Disables computation of hard shadows (scivis)")
            // scivis renderer parameters
            , _AOsamples("SciVis::AOsamples", "Number of rays per sample to compute ambient occlusion")
            , _AOdistance("SciVis::AOdistance", "Maximum distance to consider for ambient occlusion")
            // pathtracer renderer parameters
            , _rd_ptBackground("PathTracer::BackgroundTexture",
                "Texture image used as background, replacing visible lights in infinity")
            // Use depth buffer component
            , _useDB("useDBcomponent", "activates depth composition with OpenGL content")
            , _deviceTypeSlot("device", "Set the type of the OSPRay device")
            , _numThreads("numThreads", "Number of threads used for rendering") {


        core::param::EnumParam* rdt = new core::param::EnumParam(SCIVIS);
        rdt->SetTypePair(SCIVIS, "SciVis");
        rdt->SetTypePair(PATHTRACER, "PathTracer");
        rdt->SetTypePair(MPI_RAYCAST, "MPI_Raycast");

        // Ambient parameters
        this->_AOsamples << new core::param::IntParam(1);
        this->_AOdistance << new core::param::FloatParam(1e20f);
        this->_accumulateSlot << new core::param::BoolParam(true);
        this->MakeSlotAvailable(&this->_AOsamples);
        this->MakeSlotAvailable(&this->_AOdistance);
        this->MakeSlotAvailable(&this->_accumulateSlot);


        // General Renderer
        this->_rd_spp << new core::param::IntParam(1);
        this->_rd_maxRecursion << new core::param::IntParam(10);
        this->_rd_type << rdt;
        this->MakeSlotAvailable(&this->_rd_spp);
        this->MakeSlotAvailable(&this->_rd_maxRecursion);
        this->MakeSlotAvailable(&this->_rd_type);
        this->_shadows << new core::param::BoolParam(0);
        this->MakeSlotAvailable(&this->_shadows);

        this->_rd_type.ForceSetDirty(); //< TODO HAZARD Dirty hack

        // PathTracer
        this->_rd_ptBackground << new core::param::FilePathParam("");
        this->MakeSlotAvailable(&this->_rd_ptBackground);

        // Number of threads
        this->_numThreads << new core::param::IntParam(0);
        this->MakeSlotAvailable(&this->_numThreads);

        // Depth
        this->_useDB << new core::param::BoolParam(true);
        this->MakeSlotAvailable(&this->_useDB);

        // Device
        auto deviceEp = new megamol::core::param::EnumParam(deviceType::DEFAULT);
        deviceEp->SetTypePair(deviceType::DEFAULT, "cpu");
        deviceEp->SetTypePair(deviceType::MPI_DISTRIBUTED, "mpi_distributed");
        this->_deviceTypeSlot << deviceEp;
        this->MakeSlotAvailable(&this->_deviceTypeSlot);
    }

    void AbstractOSPRayRenderer::renderTexture2D(vislib::graphics::gl::GLSLShader& shader, const uint32_t* fb,
        const float* db, int& width, int& height, megamol::core::view::CallRender3D_2& cr) {

        auto fbo = cr.FrameBufferObject();
        // if (fbo != NULL) {

        //    if (fbo->IsValid()) {
        //        if ((fbo->GetWidth() != width) || (fbo->GetHeight() != height)) {
        //            fbo->Release();
        //        }
        //    }
        //    if (!fbo->IsValid()) {
        //        fbo->Create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE,
        //            vislib::graphics::gl::FramebufferObject::ATTACHMENT_TEXTURE, GL_DEPTH_COMPONENT);
        //    }
        //    if (fbo->IsValid() && !fbo->IsEnabled()) {
        //        fbo->Enable();
        //    }

        //    fbo->BindColourTexture();
        //    glClear(GL_COLOR_BUFFER_BIT);
        //    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb);
        //    glBindTexture(GL_TEXTURE_2D, 0);

        //    fbo->BindDepthTexture();
        //    glClear(GL_DEPTH_BUFFER_BIT);
        //    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, db);
        //    glBindTexture(GL_TEXTURE_2D, 0);

        //    if (fbo->IsValid()) {
        //        fbo->Disable();
        //        // fbo->DrawColourTexture();
        //        // fbo->DrawDepthTexture();
        //    }
        //} else {
        /*
        if (this->new_fbo.IsValid()) {
            if ((this->new_fbo.GetWidth() != width) || (this->new_fbo.GetHeight() != height)) {
                this->new_fbo.Release();
            }
        }
        if (!this->new_fbo.IsValid()) {
            this->new_fbo.Create(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE,
        vislib::graphics::gl::FramebufferObject::ATTACHMENT_TEXTURE, GL_DEPTH_COMPONENT);
        }
        if (this->new_fbo.IsValid() && !this->new_fbo.IsEnabled()) {
            this->new_fbo.Enable();
        }

        this->new_fbo.BindColourTexture();
        glClear(GL_COLOR_BUFFER_BIT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb);
        glBindTexture(GL_TEXTURE_2D, 0);

        this->new_fbo.BindDepthTexture();
        glClear(GL_DEPTH_BUFFER_BIT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, db);
        glBindTexture(GL_TEXTURE_2D, 0);


        glBlitNamedFramebuffer(this->new_fbo.GetID(), 0, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        this->new_fbo.Disable();
        */
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, this->_depth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, db);
        glBindTexture(GL_TEXTURE_2D, 0);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->_tex);
        glUniform1i(shader.ParameterLocation("tex"), 0);


        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, this->_depth);
        glUniform1i(shader.ParameterLocation("depth"), 1);


        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        //  }
    }


    void AbstractOSPRayRenderer::setupTextureScreen() {
        // setup color texture
        glEnable(GL_TEXTURE_2D);
        glGenTextures(1, &this->_tex);
        glBindTexture(GL_TEXTURE_2D, this->_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        //// setup depth texture
        glGenTextures(1, &this->_depth);
        glBindTexture(GL_TEXTURE_2D, this->_depth);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }

    void AbstractOSPRayRenderer::releaseTextureScreen() {
        glDeleteTextures(1, &this->_tex);
        glDeleteTextures(1, &this->_depth);
    }


    void AbstractOSPRayRenderer::initOSPRay() {
        if (!_device) {
            ospLoadModule("ispc");
            switch (this->_deviceTypeSlot.Param<megamol::core::param::EnumParam>()->Value()) {
            case deviceType::MPI_DISTRIBUTED: {
                ospLoadModule("mpi");
                _device = std::make_shared<::ospray::cpp::Device>("mpi_distributed");
                _device->setParam("masterRank", 0);
                if (this->_numThreads.Param<megamol::core::param::IntParam>()->Value() > 0) {
                    _device->setParam("numThreads", this->_numThreads.Param<megamol::core::param::IntParam>()->Value());
                }
            } break;
            default: {
                _device = std::make_shared<::ospray::cpp::Device>("cpu");
                if (this->_numThreads.Param<megamol::core::param::IntParam>()->Value() > 0) {
                    _device->setParam("numThreads", this->_numThreads.Param<megamol::core::param::IntParam>()->Value());
                } else {
                    //_device->setParam("numThreads", vislib::sys::SystemInformation::ProcessorCount() - 1);
                }
            }
            }
            _device->commit();
            _device->setCurrent();
        }
        // this->deviceTypeSlot.MakeUnavailable(); //< TODO: Make sure you can set a device only once
    }


    void AbstractOSPRayRenderer::setupOSPRay(const char* renderer_name) {
        // create and setup renderer
        _renderer = std::make_shared<::ospray::cpp::Renderer>(renderer_name);
        _camera = std::make_shared<::ospray::cpp::Camera>("perspective");
        _world = std::make_shared<::ospray::cpp::World>();
    }


    ::ospray::cpp::Texture AbstractOSPRayRenderer::TextureFromFile(vislib::TString fileName) {

        fileName = vislib::sys::Path::Resolve(fileName);

        vislib::TString ext = vislib::TString("");
        size_t pos = fileName.FindLast('.');
        if (pos != std::string::npos)
            ext = fileName.Substring(pos + 1);

        FILE* file = fopen(vislib::StringA(fileName).PeekBuffer(), "rb");
        if (!file)
            throw std::runtime_error("Could not read file");


        if (ext == vislib::TString("ppm")) {
            try {
                int rc, peekchar;

                const int LINESZ = 10000;
                char lineBuf[LINESZ + 1];

                // read format specifier:
                int format = 0;
                rc = fscanf(file, "P%i\n", &format);
                if (format != 6)
                    throw std::runtime_error("Wrong PPM format.");

                // skip all comment lines
                peekchar = getc(file);
                while (peekchar == '#') {
                    auto tmp = fgets(lineBuf, LINESZ, file);
                    (void) tmp;
                    peekchar = getc(file);
                }
                ungetc(peekchar, file);

                // read width and height from first non-comment line
                int width = -1, height = -1;
                rc = fscanf(file, "%i %i\n", &width, &height);
                if (rc != 2)
                    throw std::runtime_error("Could not read PPM width and height.");

                // skip all comment lines
                peekchar = getc(file);
                while (peekchar == '#') {
                    auto tmp = fgets(lineBuf, LINESZ, file);
                    (void) tmp;
                    peekchar = getc(file);
                }
                ungetc(peekchar, file);

                // read maxval
                int maxVal = -1;
                rc = fscanf(file, "%i", &maxVal);
                peekchar = getc(file);

                unsigned char* data;
                data = new unsigned char[width * height * 3];
                rc = fread(data, width * height * 3, 1, file);
                // flip in y, because OSPRay's textures have the origin at the lower left corner
                unsigned char* texels = (unsigned char*) data;
                for (int y = 0; y < height / 2; y++)
                    for (int x = 0; x < width * 3; x++)
                        std::swap(texels[y * width * 3 + x], texels[(height - 1 - y) * width * 3 + x]);

                ::ospray::cpp::Texture ret_tex("texture2d");
                ret_tex.setParam("format", OSP_TEXTURE_RGB8);
                ::ospray::cpp::Data<false> texel_data(texels, OSP_UCHAR,width*height);

                return ret_tex;

            } catch (std::runtime_error e) { std::cerr << e.what() << std::endl; }
        } else {
            std::cerr << "File type not supported. Only PPM file format allowed." << std::endl;
        }
    }

    bool AbstractOSPRayRenderer::AbstractIsDirty() {
        if (this->_AOsamples.IsDirty() || this->_AOdistance.IsDirty() ||
            this->_accumulateSlot.IsDirty() || this->_shadows.IsDirty() || this->_rd_type.IsDirty() ||
            this->_rd_spp.IsDirty() || this->_rd_maxRecursion.IsDirty() ||
            this->_rd_ptBackground.IsDirty() || this->_useDB.IsDirty()) {
            return true;
        } else {
            return false;
        }
    }

    void AbstractOSPRayRenderer::AbstractResetDirty() {
        this->_AOsamples.ResetDirty();
        this->_AOdistance.ResetDirty();
        this->_accumulateSlot.ResetDirty();
        this->_shadows.ResetDirty();
        this->_rd_type.ResetDirty();
        this->_rd_spp.ResetDirty();
        this->_rd_maxRecursion.ResetDirty();
        this->_rd_ptBackground.ResetDirty();
        this->_useDB.ResetDirty();
    }


    void AbstractOSPRayRenderer::fillLightArray(std::array<float,4> eyeDir) {

        // create custom ospray light
        ::ospray::cpp::Light light;

        this->_lightArray.clear();

        for (auto const& entry : this->lightMap) {
            auto const& lc = entry.second;

            switch (lc.lightType) {
            case core::view::light::lightenum::NONE:
                light = NULL;
                break;
            case core::view::light::lightenum::DISTANTLIGHT:
                light = ::ospray::cpp::Light("distant");
                if (lc.dl_eye_direction == true) {
                    light.setParam("direction", convertToVec4f(eyeDir));
                } else {
                    light.setParam("direction", convertToVec3f(lc.dl_direction));
                }
                light.setParam("angularDiameter", lc.dl_angularDiameter);
                break;
            case core::view::light::lightenum::POINTLIGHT:
                light = ::ospray::cpp::Light("point");
                light.setParam("position", convertToVec3f(lc.pl_position));
                light.setParam("radius", lc.pl_radius);
                break;
            case core::view::light::lightenum::SPOTLIGHT:
                light = ::ospray::cpp::Light("spot");
                light.setParam("position", convertToVec3f(lc.sl_position));
                light.setParam("direction", convertToVec3f(lc.sl_direction));
                light.setParam("openingAngle", lc.sl_openingAngle);
                light.setParam("penumbraAngle", lc.sl_penumbraAngle);
                light.setParam("radius", lc.sl_radius);
                break;
            case core::view::light::lightenum::QUADLIGHT:
                light = ::ospray::cpp::Light("quad");
                light.setParam("position", convertToVec3f(lc.ql_position));
                light.setParam("edge1", convertToVec3f(lc.ql_edgeOne));
                light.setParam("edge2", convertToVec3f(lc.ql_edgeTwo));
                break;
            case core::view::light::lightenum::HDRILIGHT:
                light = ::ospray::cpp::Light("hdri");
                light.setParam("up", convertToVec3f(lc.hdri_up));
                light.setParam("dir", convertToVec3f(lc.hdri_direction));
                if (lc.hdri_evnfile != vislib::TString("")) {
                    ::ospray::cpp::Texture hdri_tex = this->TextureFromFile(lc.hdri_evnfile);
                    _renderer->setParam("map_backplate", hdri_tex);
                }
                break;
            case core::view::light::lightenum::AMBIENTLIGHT:
                light =::ospray::cpp::Light("ambient");
                break;
            }
            if (lc.isValid && light != NULL) {
                light.setParam("intensity", lc.lightIntensity);
                light.setParam("color", convertToVec4f(lc.lightColor));
                light.commit();
                _lightArray.emplace_back(light);
            }
        }
    }


    void AbstractOSPRayRenderer::RendererSettings(glm::vec4 bg_color) {
        // general renderer settings
        _renderer->setParam("pixelSamples", this->_rd_spp.Param<core::param::IntParam>()->Value());
        _renderer->setParam("maxPathLength", this->_rd_maxRecursion.Param<core::param::IntParam>()->Value());

         if (this->_rd_ptBackground.Param<core::param::FilePathParam>()->Value() != vislib::TString("")) {
            ::ospray::cpp::Texture bkgnd_tex =
                this->TextureFromFile(this->_rd_ptBackground.Param<core::param::FilePathParam>()->Value());
            _renderer->setParam("map_backplate", bkgnd_tex);
        } else {
            _renderer->setParam("backgroundColor", convertToVec4f(bg_color));
        }

        switch (this->_rd_type.Param<core::param::EnumParam>()->Value()) {
        case SCIVIS:
            // scivis renderer settings
            _renderer->setParam("aoSamples", this->_AOsamples.Param<core::param::IntParam>()->Value());
            _renderer->setParam("shadows", this->_shadows.Param<core::param::BoolParam>()->Value());
            _renderer->setParam("aoDistance", this->_AOdistance.Param<core::param::FloatParam>()->Value());
            break;
        case PATHTRACER:
            _renderer->setParam("backgroundRefraction", true);
            // TODO: _renderer->setParam("roulettePathLength", );
            break;
        }
    }


    void AbstractOSPRayRenderer::setupOSPRayCamera(megamol::core::view::Camera_2& mmcam) {


        // calculate image parts for e.g. screenshooter
        std::array<float, 2> imgStart = {0, 0};
        std::array<float, 2> imgEnd = {0, 0};
        imgStart[0] = mmcam.image_tile().left() / static_cast<float>(mmcam.resolution_gate().width());
        imgStart[1] = mmcam.image_tile().bottom() / static_cast<float>(mmcam.resolution_gate().height());

        imgEnd[0] = (mmcam.image_tile().left() + mmcam.image_tile().width()) /
                    static_cast<float>(mmcam.resolution_gate().width());
        imgEnd[1] = (mmcam.image_tile().bottom() + mmcam.image_tile().height()) /
                    static_cast<float>(mmcam.resolution_gate().height());

        // setup ospcam
        _camera->setParam("imageStart", convertToVec2f(imgStart));
        _camera->setParam("imageEnd", convertToVec2f(imgEnd));
        _camera->setParam("aspect", static_cast<float>(mmcam.resolution_gate_aspect()));
        _camera->setParam("nearClip", static_cast<float>(mmcam.near_clipping_plane()));

        glm::vec4 eye_pos = mmcam.eye_position();
        glm::vec4 view_vec = mmcam.view_vector();
        glm::vec4 up_vec = mmcam.up_vector();
        _camera->setParam("position", convertToVec3f(eye_pos));
        _camera->setParam("direction", convertToVec3f(view_vec));
        _camera->setParam("up", convertToVec3f(up_vec));
        _camera->setParam("fovy", static_cast<float>(mmcam.aperture_angle()));

        // ospSet1i(_camera, "architectural", 1);
         // TODO: ospSet1f(_camera, "apertureRadius", );
        // TODO: ospSet1f(_camera, "focalDistance", cr->GetCameraParameters()->FocalDistance());
    }


    AbstractOSPRayRenderer::~AbstractOSPRayRenderer(void) {
        this->Release();
    }

    // helper function to write the rendered image as PPM file
    void AbstractOSPRayRenderer::writePPM(std::string fileName, const std::array<int,2>& size, const uint32_t* pixel) {
        // std::ofstream file;
        // file << "P6\n" << size.x << " " << size.y << "\n255\n";
        FILE* file = fopen(fileName.c_str(), "wb");
        fprintf(file, "P6\n%i %i\n255\n", size[0], size[1]);
        unsigned char* out = (unsigned char*) alloca(3 * size[0]);
        for (int y = 0; y < size[1]; y++) {
            const unsigned char* in = (const unsigned char*) &pixel[(size[1] - 1 - y) * size[1]];
            for (int x = 0; x < size[0]; x++) {
                out[3 * x + 0] = in[4 * x + 0];
                out[3 * x + 1] = in[4 * x + 1];
                out[3 * x + 2] = in[4 * x + 2];
            }
            fwrite(out, 3 * size[0], sizeof(char), file);
        }
        fprintf(file, "\n");
        fclose(file);
    }

    void AbstractOSPRayRenderer::fillMaterialContainer(CallOSPRayStructure* entry_first, const OSPRayStructureContainer& element) {
        switch (element.materialContainer->materialType) {
        case OBJMATERIAL:
            _materials[entry_first] = ::ospray::cpp::Material(this->_rd_type_string.c_str(), "obj");
            _materials[entry_first].setParam("Kd", convertToVec3f(element.materialContainer->Kd));
            _materials[entry_first].setParam("Ks", convertToVec3f(element.materialContainer->Ks));
            _materials[entry_first].setParam("Ns", element.materialContainer->Ns);
            _materials[entry_first].setParam("d", element.materialContainer->d);
            _materials[entry_first].setParam("Tf", convertToVec3f(element.materialContainer->Tf));
            break;
        case LUMINOUS:
            _materials[entry_first] = ::ospray::cpp::Material(_rd_type_string.c_str(), "luminous");
            _materials[entry_first].setParam("color", convertToVec3f(element.materialContainer->lumColor));
            _materials[entry_first].setParam("intensity", element.materialContainer->lumIntensity);
            _materials[entry_first].setParam("transparency", element.materialContainer->lumTransparency);
            break;
        case GLASS:
            _materials[entry_first] = ::ospray::cpp::Material(_rd_type_string.c_str(), "glass");
            _materials[entry_first].setParam("etaInside", element.materialContainer->glassEtaInside);
            _materials[entry_first].setParam("etaOutside", element.materialContainer->glassEtaOutside);
            _materials[entry_first].setParam(
                "attenuationColorInside", convertToVec3f(element.materialContainer->glassAttenuationColorInside));
            _materials[entry_first].setParam(
                "attenuationColorOutside", convertToVec3f(element.materialContainer->glassAttenuationColorOutside));
            _materials[entry_first].setParam(
                "attenuationDistance", element.materialContainer->glassAttenuationDistance);
            break;
        case MATTE:
            _materials[entry_first] = ::ospray::cpp::Material(_rd_type_string.c_str(), "Matte");
            _materials[entry_first].setParam(
                "reflectance", convertToVec3f(element.materialContainer->matteReflectance));
            break;
        case METAL:
            _materials[entry_first] = ::ospray::cpp::Material(_rd_type_string.c_str(), "metal");
            _materials[entry_first].setParam(
                "reflectance", convertToVec3f(element.materialContainer->metalReflectance));
            _materials[entry_first].setParam("eta", convertToVec3f(element.materialContainer->metalEta));
            _materials[entry_first].setParam("k", convertToVec3f(element.materialContainer->metalK));
            _materials[entry_first].setParam("roughness", element.materialContainer->metalRoughness);
            break;
        case METALLICPAINT:
            _materials[entry_first] = ::ospray::cpp::Material(_rd_type_string.c_str(), "metallicPaint");
            _materials[entry_first].setParam(
                "shadeColor", convertToVec3f(element.materialContainer->metallicShadeColor));
            _materials[entry_first].setParam(
                "glitterColor", convertToVec3f(element.materialContainer->metallicGlitterColor));
            _materials[entry_first].setParam("glitterSpread", element.materialContainer->metallicGlitterSpread);
            _materials[entry_first].setParam("eta", element.materialContainer->metallicEta);
            break;
        case PLASTIC:
            _materials[entry_first] = ::ospray::cpp::Material(_rd_type_string.c_str(), "Plastic");
            _materials[entry_first].setParam(
                "pigmentColor", convertToVec3f(element.materialContainer->plasticPigmentColor));
            _materials[entry_first].setParam("eta", element.materialContainer->plasticEta);
            _materials[entry_first].setParam("roughness", element.materialContainer->plasticRoughness);
            _materials[entry_first].setParam("thickness", element.materialContainer->plasticThickness);
            break;
        case THINGLASS:
            _materials[entry_first] = ::ospray::cpp::Material(_rd_type_string.c_str(), "thinGlass");
            _materials[entry_first].setParam(
                "transmission", convertToVec3f(element.materialContainer->thinglassTransmission));
            _materials[entry_first].setParam("eta", element.materialContainer->thinglassEta);
            _materials[entry_first].setParam("thickness", element.materialContainer->thinglassThickness);
            break;
        case VELVET:
            _materials[entry_first] = ::ospray::cpp::Material(_rd_type_string.c_str(), "Velvet");
            _materials[entry_first].setParam(
                "reflectance", convertToVec3f(element.materialContainer->velvetReflectance));
            _materials[entry_first].setParam(
                "horizonScatteringColor", convertToVec3f(element.materialContainer->velvetHorizonScatteringColor));
            _materials[entry_first].setParam("backScattering", element.materialContainer->velvetBackScattering);
            _materials[entry_first].setParam(
                "horizonScatteringFallOff", element.materialContainer->velvetHorizonScatteringFallOff);
            break;
        }        
    }

    void AbstractOSPRayRenderer::changeMaterial() {

        for (auto entry : this->_structureMap) {
            auto const& element = entry.second;

            // custom material settings
            if (this->_materials[entry.first] != NULL) {
                //ospRelease(this->_materials[entry.first]);
                this->_materials.erase(entry.first);
            }
            if (element.materialContainer != NULL) {
                fillMaterialContainer(entry.first, element);
                _materials[entry.first].commit();
            }

            if (this->_materials[entry.first] != NULL) {
                if (element.type == structureTypeEnum::GEOMETRY) {
                    _geometricModels[entry.first].back().setParam("material", ::ospray::cpp::CopiedData(_materials[entry.first]));
                    _geometricModels[entry.first].back().commit();
                }
            }
        }
    }

    void AbstractOSPRayRenderer::changeTransformation() {

        for (auto& entry : this->_baseStructures) {
            if (this->_structureMap[entry.first].transformationContainer == nullptr)
                continue;
            auto trafo = this->_structureMap[entry.first].transformationContainer;
            ::rkcommon::math::affine3f xfm;
            xfm.p.x = trafo->pos[0];
            xfm.p.y = trafo->pos[1];
            xfm.p.z = trafo->pos[2];
            xfm.l.vx.x = trafo->MX[0][0];
            xfm.l.vx.y = trafo->MX[0][1];
            xfm.l.vx.z = trafo->MX[0][2];
            xfm.l.vy.x = trafo->MX[1][0];
            xfm.l.vy.y = trafo->MX[1][1];
            xfm.l.vy.z = trafo->MX[1][2];
            xfm.l.vz.x = trafo->MX[2][0];
            xfm.l.vz.y = trafo->MX[2][1];
            xfm.l.vz.z = trafo->MX[2][2];

            _instances[entry.first].setParam("xfm", xfm);
            _instances[entry.first].commit();
        }
    }


    bool AbstractOSPRayRenderer::generateRepresentations() {

        bool returnValue = true;
        bool applyTransformation = false;

        ::rkcommon::math::box3f _worldBounds;
        std::vector<::rkcommon::math::box3f> ghostRegions;
        std::vector<::rkcommon::math::box3f> regions;

        for (auto& entry : this->_structureMap) {

            _numCreateGeo = 1;
            auto const& element = entry.second;

            // check if structure should be released first
            if (element.dataChanged) {
                //for (auto& stru : this->_baseStructures[entry.first]) {
                //    if (element.type == structureTypeEnum::GEOMETRY) {
                //        ospRemoveParam(this->_world, std::get<::ospray::cpp::Geometry>(stru));
                //        // ospRelease(std::get<::ospray::cpp::Geometry>(stru));
                //    } else if (element.type == structureTypeEnum::VOLUME) {
                //        ospRemoveVolume(this->_world, std::get<::ospray::cpp::Volume>(stru));
                //        // ospRelease(std::get<::ospray::cpp::Volume>(stru));
                //    }
                //}
                this->_baseStructures.erase(entry.first);
            } else {
                continue;
            }


            // custom material settings
            if (_materials[entry.first]) {
                _materials.erase(entry.first);
            }
            if (element.materialContainer &&
                this->_rd_type.Param<megamol::core::param::EnumParam>()->Value() != MPI_RAYCAST) {
                fillMaterialContainer(entry.first, element);
                _materials[entry.first].commit();
            }

            switch (element.type) {
            case structureTypeEnum::UNINITIALIZED:
                break;

            case structureTypeEnum::OSPRAY_API_STRUCTURES:
                if (element.ospStructures.first.empty()) {
                    // returnValue = false;
                    break;
                }

                for (auto structure : element.ospStructures.first) {
                    if (element.ospStructures.second == structureTypeEnum::GEOMETRY) {
                        _baseStructures[entry.first].emplace_back(reinterpret_cast<OSPGeometry>(structure));
                        _geometricModels[entry.first].emplace_back(::ospray::cpp::GeometricModel(std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())));
                        //_geometricModels[entry.first].back().commit();
                    } else if (element.ospStructures.second == structureTypeEnum::VOLUME) {
                        _baseStructures[entry.first].emplace_back(reinterpret_cast<OSPVolume>(structure));
                        _volumetricModels[entry.first].emplace_back(::ospray::cpp::VolumetricModel(
                            std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back())));
                        _volumetricModels[entry.first].back().commit();
                    } else {
                        megamol::core::utility::log::Log::DefaultLog.WriteError(
                            "OSPRAY_API_STRUCTURE: Something went wrong.");
                        break;
                    }
                }

                // General geometry execution
                for (unsigned int i = 0; i < element.ospStructures.first.size(); i++) {
                    auto idx = _baseStructures[entry.first].size() - 1 - i;
                    if (_materials[entry.first] != NULL && _baseStructures[entry.first].size() > 0) {
                        _geometricModels[entry.first][idx].setParam("material", ::ospray::cpp::CopiedData(_materials[entry.first]));
                        _geometricModels[entry.first][idx].commit();
                    }
                }

                _groups[entry.first] = ::ospray::cpp::Group();
                if (!_geometricModels[entry.first].empty()) {
                    _groups[entry.first].setParam("geometry", ::ospray::cpp::CopiedData(_geometricModels[entry.first]));
                }
                if (!_volumetricModels[entry.first].empty()) {
                    _groups[entry.first].setParam("volume", ::ospray::cpp::CopiedData(_volumetricModels[entry.first]));
                }
                _groups[entry.first].commit();
                break;
            case structureTypeEnum::GEOMETRY:
                switch (element.geometryType) {
                case geometryTypeEnum::TEST:

                    using namespace rkcommon::math;
                    using namespace ::ospray::cpp;
                    {
                    // triangle mesh data
                    std::vector<vec3f> vertex = {vec3f(-1.0f, -1.0f, 3.0f), vec3f(-1.0f, 1.0f, 3.0f),
                        vec3f(1.0f, -1.0f, 3.0f), vec3f(0.1f, 0.1f, 0.3f)};

                    std::vector<vec4f> color = {vec4f(0.9f, 0.5f, 0.5f, 1.0f), vec4f(0.8f, 0.8f, 0.8f, 1.0f),
                        vec4f(0.8f, 0.8f, 0.8f, 1.0f), vec4f(0.5f, 0.9f, 0.5f, 1.0f)};

                    std::vector<vec3ui> index = {vec3ui(0, 1, 2), vec3ui(1, 2, 3)};

                        // create and setup model and mesh
                    _baseStructures[entry.first].emplace_back(::ospray::cpp::Geometry("mesh"));
                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("vertex.position", CopiedData(vertex));
                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("vertex.color", CopiedData(color));
                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("index", CopiedData(index));
                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).commit();

                    // put the mesh into a model
                    _geometricModels[entry.first].emplace_back(::ospray::cpp::GeometricModel(
                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())));
                    _geometricModels[entry.first].back().commit();
                    }

                    break;
                case geometryTypeEnum::SPHERES:
                    if (element.vertexData == NULL) {
                        // returnValue = false;
                        break;
                    }

                    _numCreateGeo = element.partCount * element.vertexLength * sizeof(float) / _ispcLimit + 1;

                    for (unsigned int i = 0; i < _numCreateGeo; i++) {
                        _baseStructures[entry.first].emplace_back(::ospray::cpp::Geometry("sphere"));

                        unsigned long long vertexFloatsToRead = element.partCount * element.vertexLength / _numCreateGeo;
                        vertexFloatsToRead -= vertexFloatsToRead % element.vertexLength;

                        unsigned long long bytes_per_sphere = element.vertexLength * sizeof(float);
                        auto vertexData = ::ospray::cpp::SharedData(&element.vertexData->operator[](i* vertexFloatsToRead), OSP_VEC3F, vertexFloatsToRead/element.vertexLength, bytes_per_sphere);

                        vertexData.commit();
                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())
                            .setParam("sphere.position", vertexData);

                        if (element.vertexLength > 3) {
                            auto radiusData =
                                ::ospray::cpp::SharedData(&element.vertexData->operator[](i* vertexFloatsToRead + 3),
                                    OSP_FLOAT, vertexFloatsToRead / element.vertexLength, bytes_per_sphere);
                            radiusData.commit();
                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("sphere.radius", radiusData);
                        } else {
                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("radius", element.globalRadius);
                        }

                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).commit();

                        _geometricModels[entry.first].emplace_back(::ospray::cpp::GeometricModel(std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())));

                        if (element.colorLength == 4) {
                            unsigned long long colorFloatsToRead = element.partCount * element.colorLength / _numCreateGeo;
                            colorFloatsToRead -= colorFloatsToRead % element.colorLength;

                            auto colorData =
                                ::ospray::cpp::SharedData(&element.colorData->operator[](i* colorFloatsToRead),
                                    OSP_VEC4F, colorFloatsToRead / element.colorLength);
                            colorData.commit();
                            _geometricModels[entry.first].back().setParam("color", colorData);
                        }
                    }
                    break;

                case geometryTypeEnum::NHSPHERES:
                    if (element.raw == NULL) {
                        // returnValue = false;
                        break;
                    }

                    _numCreateGeo = element.partCount * element.dataStride / _ispcLimit + 1;

                    for (unsigned int i = 0; i < _numCreateGeo; i++) {
                        _baseStructures[entry.first].push_back(::ospray::cpp::Geometry("sphere"));


                        unsigned long long floatsToRead =
                            element.partCount * element.dataStride / (_numCreateGeo * sizeof(float));
                        floatsToRead -= floatsToRead % (element.dataStride / sizeof(float));
                        unsigned long long bytes_per_sphere = element.dataStride;
                        unsigned long long floats_per_sphere = element.dataStride / sizeof(float);

                        auto vertexData =
                            ::ospray::cpp::SharedData(&static_cast<const float*>(element.raw)[i * floatsToRead],
                                OSP_VEC3F, floatsToRead / floats_per_sphere, bytes_per_sphere);
                        vertexData.commit();

                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("sphere.position", vertexData);

                        if (element.vertexLength > 3) {
                            auto radiusData = ::ospray::cpp::SharedData(&static_cast<const float*>(element.raw)[i * floatsToRead + 3],
                                    OSP_FLOAT, floatsToRead / floats_per_sphere, bytes_per_sphere);
                            radiusData.commit();
                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("sphere.radius",
                                radiusData);
                        } else {
                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("radius",
                                element.globalRadius);
                        }

                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).commit();
                        _geometricModels[entry.first].emplace_back(::ospray::cpp::GeometricModel(
                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())));
                        if (element.mmpldColor ==
                                core::moldyn::SimpleSphericalParticles::ColourDataType::COLDATA_FLOAT_RGBA) {

                            auto colorData = ::ospray::cpp::SharedData(
                                &static_cast<const float*>(element.raw)[i * floatsToRead + element.vertexLength],
                                OSP_VEC4F, floatsToRead / floats_per_sphere, bytes_per_sphere);
                            colorData.commit();
                            _geometricModels[entry.first].back().setParam("color", colorData);
                        } else if (element.mmpldColor ==
                            core::moldyn::SimpleSphericalParticles::ColourDataType::COLDATA_FLOAT_RGB) {
                            megamol::core::utility::log::Log::DefaultLog.WriteError("OSPRAY_NHSPERES: RGB not supported.");
                            rkcommon::math::vec4f gcol = {0.8, 0.8, 0.8, 1.0};
                            _geometricModels[entry.first].back().setParam("color", gcol);
                        } else if (element.mmpldColor ==
                                   core::moldyn::SimpleSphericalParticles::ColourDataType::COLDATA_NONE) {
                            // TODO: get global color
                            rkcommon::math::vec4f gcol = {0.8, 0.8, 0.8, 1.0};
                            _geometricModels[entry.first].back().setParam("color", gcol);
                        } // end if color type
                    } // end for num geometies
                    break;
                case geometryTypeEnum::MESH:
                    if (element.mesh == NULL) {
                        // returnValue = false;
                        break;
                    }
                    {
                        std::vector<mesh::ImageDataAccessCollection::Image> tex_vec;
                        if (element.mesh_textures != nullptr) {
                            assert(element.mesh->accessMeshes().size() == element.mesh_textures->accessImages().size());
                            tex_vec = element.mesh_textures->accessImages();
                        }
                        _numCreateGeo = element.mesh->accessMeshes().size();

                        uint32_t mesh_index = 0;
                        for (auto& mesh : element.mesh->accessMeshes()) {

                            this->_baseStructures[entry.first].emplace_back(ospNewGeometry("mesh"));

                            auto mesh_type = mesh.second.primitive_type;

                            for (auto& attrib : mesh.second.attributes) {

                                if (attrib.semantic == mesh::MeshDataAccessCollection::POSITION) {
                                    auto count = attrib.byte_size /
                                                 (mesh::MeshDataAccessCollection::getByteSize(attrib.component_type) *
                                                     attrib.component_cnt);

                                    auto vertexData =
                                        ::ospray::cpp::SharedData(attrib.data, OSP_VEC3F, count, attrib.stride);
                                    vertexData.commit();
                                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("vertex.position", vertexData);
                                }

                                // check normal pointer
                                if (attrib.semantic == mesh::MeshDataAccessCollection::NORMAL) {
                                    auto count = attrib.byte_size / attrib.stride;
                                    auto normalData =
                                        ::ospray::cpp::SharedData(attrib.data, OSP_VEC3F, count, attrib.stride);
                                    normalData.commit();
                                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("vertex.normal", normalData);
                                }

                                // check colorpointer and convert to rgba
                                if (attrib.semantic == mesh::MeshDataAccessCollection::COLOR) {
                                    ::ospray::cpp::SharedData colorData;
                                    if (attrib.component_type == mesh::MeshDataAccessCollection::ValueType::FLOAT) {
                                        auto count = attrib.byte_size / (mesh::MeshDataAccessCollection::getByteSize(attrib.component_type) * attrib.component_cnt);
                                        auto osp_type = OSP_VEC3F;
                                        if (attrib.component_cnt == 4) osp_type = OSP_VEC4F;
                                        colorData =
                                            ::ospray::cpp::SharedData(attrib.data, osp_type, count, attrib.stride);
                                    } else {
                                        core::utility::log::Log::DefaultLog.WriteError("[OSPRayRenderer][MESH] Color type not supported.");
                                    }
                                    colorData.commit();
                                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("vertex.color", colorData);
                                }

                                // check texture array
                                if (attrib.semantic == mesh::MeshDataAccessCollection::TEXCOORD) {
                                    auto count = attrib.byte_size /
                                                 (mesh::MeshDataAccessCollection::getByteSize(attrib.component_type) *
                                                     attrib.component_cnt);
                                    auto texData = ::ospray::cpp::SharedData(attrib.data, OSP_VEC2F, count, attrib.stride);
                                    texData.commit();
                                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("vertex.texcoord", texData);
                                }
                            }
                            // check index pointer
                            if (mesh.second.indices.data != nullptr) {
                                auto count = mesh.second.indices.byte_size /
                                             mesh::MeshDataAccessCollection::getByteSize(mesh.second.indices.type);

                                unsigned long long stride = 3 * sizeof(unsigned int);
                                auto osp_type = OSP_VEC3UI;

                                if (mesh_type == mesh::MeshDataAccessCollection::QUADS) {
                                    stride = 4 * sizeof(unsigned int);
                                    osp_type = OSP_VEC4UI;
                                }
                                
                                auto indexData =
                                    ::ospray::cpp::SharedData(mesh.second.indices.data, osp_type, count, stride);
                                indexData.commit();
                                std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("index", indexData);
                            } else {
                                megamol::core::utility::log::Log::DefaultLog.WriteError(
                                    "OSPRay cannot render meshes without index array");
                                returnValue = false;
                            }

                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).commit();
                            _geometricModels[entry.first].emplace_back(::ospray::cpp::GeometricModel(
                                std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())));

                            if (element.mesh_textures != nullptr) {
                                auto osp_tex_format = OSP_TEXTURE_FORMAT_INVALID;
                                auto osp_data_format = OSP_BYTE;
                                switch (tex_vec[mesh_index].format) {
                                case mesh::ImageDataAccessCollection::TextureFormat::RGBA8:
                                    osp_tex_format = OSP_TEXTURE_RGBA8;
                                    break;
                                case mesh::ImageDataAccessCollection::TextureFormat::RGB32F:
                                    osp_tex_format = OSP_TEXTURE_RGB32F;
                                    osp_data_format = OSP_FLOAT;
                                    break;
                                case mesh::ImageDataAccessCollection::TextureFormat::RGB8:
                                    osp_tex_format = OSP_TEXTURE_RGB8;
                                    break;
                                case mesh::ImageDataAccessCollection::TextureFormat::RGBA32F:
                                    osp_tex_format = OSP_TEXTURE_RGBA32F;
                                    osp_data_format = OSP_FLOAT;
                                    break;
                                default:
                                    osp_tex_format = OSP_TEXTURE_RGB8;
                                    break;
                                }

                                auto ospTexture = ::ospray::cpp::Texture("texture2d");
                                rkcommon::math::vec2i width_height = {tex_vec[mesh_index].width, tex_vec[mesh_index].height};

                                auto textureData = ::ospray::cpp::SharedData(
                                        tex_vec[mesh_index].data, osp_data_format, width_height);
                                textureData.commit();

                                ospTexture.setParam("format", osp_tex_format);
                                ospTexture.setParam("data", textureData);
                                ospTexture.commit();

                                auto ospMat = ::ospray::cpp::Material(_rd_type_string.c_str(), "obj");
                                ospMat.setParam("map_Kd", ospTexture);
                                // ospSetObject(ospMat, "map_Ks", ospTexture);
                                // ospSetObject(ospMat, "map_d", ospTexture);
                                ospMat.commit();
                                _geometricModels[entry.first].back().setParam("material", ::ospray::cpp::CopiedData(ospMat));
                            }
                            mesh_index++;
                        }
                    }
                    break;
                case geometryTypeEnum::LINES:
                case geometryTypeEnum::CURVES:
                    if (element.vertexData == nullptr && element.mesh == nullptr) {
                        // returnValue = false;
                        megamol::core::utility::log::Log::DefaultLog.WriteError(
                            "[AbstractOSPRayRenderer]Streamline geometry detected but no data found.");
                        break;
                    }
                    if (element.mesh != nullptr) {
                        this->_numCreateGeo = element.mesh->accessMeshes().size();
                        for (auto& mesh : element.mesh->accessMeshes()) {

                            _baseStructures[entry.first].emplace_back(::ospray::cpp::Geometry("curve"));

                            for (auto& attrib : mesh.second.attributes) {

                                if (attrib.semantic == mesh::MeshDataAccessCollection::POSITION) {
                                    const auto count = attrib.byte_size / attrib.stride;
                                    auto vertexData = ::ospray::cpp::SharedData(attrib.data, OSP_VEC3F, count, attrib.stride);
                                    vertexData.commit();
                                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("vertex.position", vertexData);
                                }

                                // check colorpointer and convert to rgba
                                if (attrib.semantic == mesh::MeshDataAccessCollection::COLOR) {
                                    ::ospray::cpp::SharedData colorData;
                                    if (attrib.component_type == mesh::MeshDataAccessCollection::ValueType::FLOAT) {
                                        auto count = attrib.byte_size / attrib.stride;
                                        colorData = ::ospray::cpp::SharedData(
                                            attrib.data, OSP_VEC3F, count, attrib.stride);
                                    } else {
                                        core::utility::log::Log::DefaultLog.WriteError("[OSPRayRenderer][CURVE] Color type not supported.");
                                    }
                                    colorData.commit();
                                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("vertex.color", colorData);
                                }
                            }
                            // check index pointer
                            if (mesh.second.indices.data != nullptr) {
                                const auto count =
                                    mesh.second.indices.byte_size /
                                    mesh::MeshDataAccessCollection::getByteSize(mesh.second.indices.type);
                                auto indexData = ::ospray::cpp::SharedData(mesh.second.indices.data, OSP_UINT, count);
                                indexData.commit();
                                std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("index", indexData);
                            } else {
                                megamol::core::utility::log::Log::DefaultLog.WriteError(
                                    "OSPRay cannot render curves without index array");
                                returnValue = false;
                            }

                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("radius", element.globalRadius);
                            // TODO: Add user input support for this
                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("type", OSP_ROUND);
                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("basis", OSP_LINEAR);

                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).commit();
                            _geometricModels[entry.first].emplace_back(::ospray::cpp::GeometricModel(
                                std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())));

                        } // end for geometry
                    } else {
                        _baseStructures[entry.first].emplace_back(::ospray::cpp::Geometry("curve"));
                        this->_numCreateGeo = 1;

                        auto vertexData = ::ospray::cpp::SharedData(element.vertexData->data(), OSP_VEC3F, element.vertexData->size() / 3);
                        vertexData.commit();
                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("vertex.position", vertexData);

                        auto indexData = ::ospray::cpp::SharedData(*element.indexData);
                        indexData.commit();
                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("index", indexData);

                        if (element.colorData->size() > 0) {
                            auto colorData = ::ospray::cpp::SharedData(element.colorData->data(), OSP_VEC4F, element.colorData->size()/4);
                            colorData.commit();
                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("vertex.color", colorData);
                        }

                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("radius", element.globalRadius);
                        // TODO: Add user input support for this
                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())
                            .setParam("type", OSP_ROUND);
                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())
                            .setParam("basis", OSP_LINEAR);

                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).commit();
                        _geometricModels[entry.first].emplace_back(::ospray::cpp::GeometricModel(
                            std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())));
                    }
                    break;
                }

                // General geometry execution
                for (unsigned int i = 0; i < this->_numCreateGeo; i++) {
                    if (_materials[entry.first] != NULL && _geometricModels[entry.first].size() > 0) {
                        _geometricModels[entry.first].rbegin()[i].setParam("material", ::ospray::cpp::CopiedData(_materials[entry.first]));
                    }

                    if (_geometricModels[entry.first].size() > 0) {
                        _geometricModels[entry.first].rbegin()[i].commit();
                    }
                    _groups[entry.first] = ::ospray::cpp::Group();
                    _groups[entry.first].setParam("geometry", ::ospray::cpp::CopiedData(_geometricModels[entry.first]));
                    _groups[entry.first].commit();
                }
                break;

            case structureTypeEnum::VOLUME:

                if (element.voxels == NULL) {
                    // returnValue = false;
                    break;
                }

                _baseStructures[entry.first].emplace_back(::ospray::cpp::Volume("structuredRegular"));

                auto type = static_cast<uint8_t>(element.voxelDType);

                 // add data
                rkcommon::math::vec3i dims = { element.dimensions[0], element.dimensions[1], element.dimensions[2]};
                auto voxelData = ::ospray::cpp::SharedData(element.voxels,
                    static_cast<OSPDataType>(voxelDataTypeOSP[type]), dims);
                voxelData.commit();


                std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()).setParam("data", voxelData);

                //ospSet3iv(std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()), "dimensions",element.dimensions->data());
                std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()).setParam("gridOrigin", convertToVec3f(element.gridOrigin));
                std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()).setParam("gridSpacing", convertToVec3f(element.gridSpacing));


                //std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()).setParam("voxelRange", element.valueRange);
                //ospSet1b(std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()), "singleShade", element.useMIP);
                //ospSet1b(std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()), "gradientShadingEnables",
                //    element.useGradient);
                //ospSet1b(std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()), "preIntegration",
                //    element.usePreIntegration);
                //ospSet1b(std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()), "adaptiveSampling",
                //    element.useAdaptiveSampling);
                //ospSet1f(
                //    std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()), "adaptiveScalar", element.adaptiveFactor);
                //ospSet1f(std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()), "adaptiveMaxSamplingRate",
                //    element.adaptiveMaxRate);
                //ospSet1f(std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()), "samplingRate", element.samplingRate);



                // ClippingBox
                auto clipping_box_geometry = ::ospray::cpp::Geometry("box");
                ::rkcommon::math::box3f box;
                box.lower = {element.clippingBoxLower[0], element.clippingBoxLower[1], element.clippingBoxLower[2]};
                box.upper = { element.clippingBoxUpper[0], element.clippingBoxUpper[1], element.clippingBoxUpper[2]};
                clipping_box_geometry.setParam("box", box);
                clipping_box_geometry.commit();
                
                if (element.clippingBoxActive) {
                    _clippingModels[entry.first].emplace_back(::ospray::cpp::GeometricModel(clipping_box_geometry));
                }

                auto tf = ::ospray::cpp::TransferFunction("piecewiseLinear");

                auto tf_rgb = ::ospray::cpp::SharedData(element.tfRGB->data(), OSP_FLOAT, element.tfRGB->size() / 3);
                auto tf_opa = ::ospray::cpp::SharedData(*(element.tfA));
                tf.setParam("color", tf_rgb);
                tf.setParam("opacity", tf_opa);
                rkcommon::math::vec2f valrange = {element.valueRange[0], element.valueRange[1]};
                tf.setParam("valueRange", valrange);

                tf.commit();

                std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back()).commit();
                _volumetricModels[entry.first].emplace_back(::ospray::cpp::VolumetricModel(
                    std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].back())));

                _volumetricModels[entry.first].back().setParam("transferFunction", tf);
                _volumetricModels[entry.first].back().commit();
 
                switch (element.volRepType) {
                case volumeRepresentationType::VOLUMEREP:
                    _groups[entry.first] = ::ospray::cpp::Group();
                    _groups[entry.first].setParam("volume", ::ospray::cpp::CopiedData(_volumetricModels[entry.first]));
                    if (element.clippingBoxActive) {
                        _groups[entry.first].setParam(
                            "clippingGeometry", ::ospray::cpp::CopiedData(_clippingModels[entry.first]));
                    }
                    _groups[entry.first].commit();
                    break;

                case volumeRepresentationType::ISOSURFACE:
                    // isosurface
                    _baseStructures[entry.first].emplace_back(::ospray::cpp::Geometry("isosurface"));
                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("isovalue", element.isoValue);
 
                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).setParam("volume", ::ospray::cpp::SharedData(std::get<::ospray::cpp::Volume>(_baseStructures[entry.first].front())));

                    std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back()).commit();
                    _geometricModels[entry.first].emplace_back(::ospray::cpp::GeometricModel(
                        std::get<::ospray::cpp::Geometry>(_baseStructures[entry.first].back())));


                    if (_materials[entry.first] != NULL) {
                        _geometricModels[entry.first].back().setParam(
                            "material", ::ospray::cpp::CopiedData(_materials[entry.first]));
                    }

                    _groups[entry.first] = ::ospray::cpp::Group();
                    _groups[entry.first].setParam("geometry", ::ospray::cpp::CopiedData(_geometricModels[entry.first]));
                    if (element.clippingBoxActive) {
                        _groups[entry.first].setParam(
                            "clippingGeometry", ::ospray::cpp::CopiedData(_clippingModels[entry.first]));
                    }
                    _groups[entry.first].commit();
                    break;

                }
                break;
            }

        } // for element loop

        //if (this->_rd_type.Param<megamol::core::param::EnumParam>()->Value() == MPI_RAYCAST && ghostRegions.size() > 0 &&
        //    regions.size() > 0) {
        //    for (auto const& el : regions) {
        //        ghostRegions.push_back(_worldBounds);
        //    }
        //    auto ghostRegionData = ospNewData(2 * ghostRegions.size(), OSP_FLOAT3, ghostRegions.data());
        //    auto regionData = ospNewData(2 * regions.size(), OSP_FLOAT3, ghostRegions.data());
        //    ospCommit(ghostRegionData);
        //    ospCommit(regionData);
        //    ospSetData(_world, "ghostRegions", ghostRegionData);
        //    ospSetData(_world, "regions", ghostRegionData);
        //}

        return returnValue;
    }

    void AbstractOSPRayRenderer::createInstances() {

        for (auto& entry : this->_structureMap) {

            auto const& element = entry.second;

            _instances[entry.first] = ::ospray::cpp::Instance(_groups[entry.first]);

            if (element.transformationContainer) {

                auto trafo = element.transformationContainer;
                ::rkcommon::math::affine3f xfm;
                xfm.p.x = trafo->pos[0];
                xfm.p.y = trafo->pos[1];
                xfm.p.z = trafo->pos[2];
                xfm.l.vx.x = trafo->MX[0][0];
                xfm.l.vx.y = trafo->MX[0][1];
                xfm.l.vx.z = trafo->MX[0][2];
                xfm.l.vy.x = trafo->MX[1][0];
                xfm.l.vy.y = trafo->MX[1][1];
                xfm.l.vy.z = trafo->MX[1][2];
                xfm.l.vz.x = trafo->MX[2][0];
                xfm.l.vz.y = trafo->MX[2][1];
                xfm.l.vz.z = trafo->MX[2][2];

                _instances[entry.first].setParam("xfm", xfm);
            }

            _instances[entry.first].commit();
        }
    }

    void AbstractOSPRayRenderer::releaseOSPRayStuff() {}

} // end namespace ospray
} // end namespace megamol
