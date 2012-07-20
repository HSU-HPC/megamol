/*
 * Console.cpp
 *
 * Copyright (C) 2006 - 2008 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */

#include "stdafx.h"
#include <cstdlib>
#include <climits>
#include <ctime>
#include <signal.h>

#include "MegaMolCore.h"
#define MEGAMOLVIEWER_SINGLEFILE
#include "MegaMolViewer.h"

#include "AboutInfo.h"
#include "CmdLineParser.h"
#include "CoreHandle.h"
#include "HotKeyCallback.h"
#include "JobManager.h"
#include "Window.h"
#include "WindowManager.h"

#include "vislib/Array.h"
#include "vislib/BufferedFile.h"
#include "vislib/CriticalSection.h"
#include "vislib/Console.h"
#include "vislib/functioncast.h"
#include "vislib/Log.h"
#include "vislib/Map.h"
#include "vislib/memutils.h"
#include "vislib/MultiSz.h"
#include "vislib/Path.h"
#include "vislib/SingleLinkedList.h"
#include "vislib/SmartPtr.h"
#include "vislib/String.h"
#include "vislib/StringTokeniser.h"
#include "vislib/sysfunctions.h"
#include "vislib/ThreadSafeStackTrace.h"
#include "vislib/Trace.h"


/**
 * The core instance handle.
 */
static megamol::console::CoreHandle hCore;


/**
 * The viewer instance handle.
 */
static megamol::console::CoreHandle hView;


/**
 * The critical section used to synchronise the console output.
 */
static vislib::sys::CriticalSection consoleWriteLock;


/**
 * The parameter file to use
 */
static vislib::TString parameterFile;


/**
 * An application wide termination request
 */
static bool terminationRequest;


/**
 * Flag if the log echo printed an important message (which will change the
 * application return value to nonzero).
 */
static bool echoedImportant;


/**
 * The path to the front end executable currently executing
 */
static vislib::TString applicationExecutablePath;


/**
 * Flag to serialize an deserialize relative file names
 */
static bool useRelativeFileNames;


extern "C" {

/**
 * Writes a log message echo to the console.
 *
 * @param level The level of the log message.
 * @param message The text of the log message.
 */
void MEGAMOLCORE_CALLBACK writeLogEchoToConsole(unsigned int level,
        const char* message) {
    static bool colourfulConsole = vislib::sys::Console::ColorsEnabled();
    consoleWriteLock.Lock();
    if (colourfulConsole) {
        if (level <= 1) {
            vislib::sys::Console::SetBackgroundColor(
                vislib::sys::Console::BLACK);
            vislib::sys::Console::SetForegroundColor(
                vislib::sys::Console::RED);
            ::echoedImportant = true;
        } else if (level <= 100) {
            vislib::sys::Console::SetBackgroundColor(
                vislib::sys::Console::BLACK);
            vislib::sys::Console::SetForegroundColor(
                vislib::sys::Console::YELLOW);
        } else if (level <= 200) {
            vislib::sys::Console::SetBackgroundColor(
                vislib::sys::Console::BLACK);
            vislib::sys::Console::SetForegroundColor(
                vislib::sys::Console::WHITE);
        }
    }
    vislib::sys::Console::Write("%4d", level);
    if (colourfulConsole) {
        vislib::sys::Console::RestoreDefaultColors();
    }
    vislib::sys::Console::Write("|%s", message);
    consoleWriteLock.Unlock();

}

}


/**
 * Yet another utility class
 */
class writeLogEchoToConsoleEchoTarget : public vislib::sys::Log::Target {
public:

    /** ctor */
    writeLogEchoToConsoleEchoTarget() : vislib::sys::Log::Target() { }

    /** dtor */
    virtual ~writeLogEchoToConsoleEchoTarget() { }

    /**
        * Writes a message to the log target
        *
        * @param level The level of the message
        * @param time The time stamp of the message
        * @param sid The object id of the source of the message
        * @param msg The message text itself
        */
    virtual void Msg(UINT level, vislib::sys::Log::TimeStamp time, vislib::sys::Log::SourceID sid,
            const char *msg) {
        writeLogEchoToConsole(level, msg);
    }

};


/**
 * Instance of the utility class
 */
static vislib::SmartPtr<vislib::sys::Log::Target> dummyEchoTarget;


/**
 * Forces the viewer library to be loaded. You must not call this method before
 * the core instance has been created and the configuration file is loaded.
 *
 * @return 'true' on success, 'false' on failure.
 */
bool forceViewerLib(void) {
    VLSTACKTRACE("forceViewerLib", __FILE__, __LINE__);
    using vislib::sys::Log;
    ASSERT(hCore.IsValid());
    bool error = false;

    if (!megamol::viewer::mmvIsLibraryLoaded()) {
        vislib::StringA libName;
        vislib::StringA libStubName = "MegaMolGlut";
        ::mmcValueType type = MMC_TYPE_VOIDP;
        const void *data = NULL;

        type = MMC_TYPE_VOIDP;
        data = ::mmcGetConfigurationValue(hCore, MMC_CFGID_VARIABLE,
            "viewerLib", &type);
        if (type == MMC_TYPE_CSTR) {
            libStubName = static_cast<const char*>(data);
        } else if (type == MMC_TYPE_WSTR) {
            libStubName = static_cast<const wchar_t*>(data);
        }

        libName.Format(megamol::console::utility::\
            AboutInfo::LibFileNameFormatString(), libStubName.PeekBuffer());
        bool loadResult = false;

        type = MMC_TYPE_VOIDP;
        data = ::mmcGetConfigurationValue(hCore, MMC_CFGID_APPLICATION_DIR,
            NULL, &type);

        if (type == MMC_TYPE_CSTR) {
            libName = vislib::sys::Path::Concatenate(
                static_cast<const char *>(data),
                libName);
            loadResult = megamol::viewer::mmvLoadLibraryA(
                libName.PeekBuffer());

        } else if (type == MMC_TYPE_WSTR) {
            vislib::StringW libNameW = vislib::sys::Path::Concatenate(
                static_cast<const wchar_t *>(data),
                vislib::StringW(libName));
            libName = vislib::StringA(libNameW);
            loadResult = megamol::viewer::mmvLoadLibraryW(
                libNameW.PeekBuffer());

        } else {
            Log::DefaultLog.WriteMsg(Log::LEVEL_WARN,
                "Application directory is not configured.\n");
            loadResult = megamol::viewer::mmvLoadLibraryA(libName);

        }

        if (loadResult) {
            vislib::SmartPtr<vislib::StackTrace> man
                = vislib::sys::ThreadSafeStackTrace::Manager();
            mmvInitStackTracer(static_cast<void*>(&man));

            Log::DefaultLog.WriteMsg(Log::LEVEL_INFO + 200,
                "Viewer loaded");
            megamol::console::utility::AboutInfo::LogViewerVersionInfo();

        } else {
            if (!vislib::sys::File::Exists(libName)) {
                Log::DefaultLog.WriteMsg(Log::LEVEL_ERROR + 10,
                    "File not found %s\n", libName.PeekBuffer());
            }
            Log::DefaultLog.WriteMsg(Log::LEVEL_ERROR,
                "Unable to load MegaMol%s Viewer", ASCIIStringTM());
            error = true;
        }
    }

    return !error;
}


template<class T>
static bool stringFSEqual(const T& lhs, const T& rhs) {
#ifdef _WIN32
    return lhs.Equals(rhs, false);
#else /* _WIN32 */
    return lhs.Equals(rhs, true);
#endif /* _WIN32 */
}


static vislib::TString relativePathTo(const vislib::TString& path,
        const vislib::TString& base) {
    TCHAR pathSep = vislib::TString(vislib::sys::Path::SEPARATOR_A, 1)[0];
    vislib::Array<vislib::TString> pa = vislib::TStringTokeniser::Split(path,
        pathSep, false);
    vislib::Array<vislib::TString> ba = vislib::TStringTokeniser::Split(base,
        pathSep, false);

    if ((pa.Count() <= 0) || (ba.Count() <= 0)) return path;
    if (!stringFSEqual(pa[0], ba[0])) return path;

    int i = 1;
    while ((i < pa.Count()) && (i < ba.Count())
        && stringFSEqual(pa[i], ba[i])) ++i; // number of similar levels.

    vislib::TString result;
    for (int j = 0; j < ba.Count() - i; ++j) {
        if (j > 0) result += pathSep;
        result += _T("..");
    }
    for (; i < pa.Count(); ++i) {
        result += pathSep;
        result += pa[i];
    }

    return result;
}


extern "C" {

/**
 * Writes one parameter name-value pair to the file stored in 'data'.
 *
 * @param The name of one parameter to store.
 * @data The file to write the info to.
 */
void MEGAMOLCORE_CALLBACK writeParameterFileParameter(const char* str,
        void *data) {
    vislib::sys::File* pfile = static_cast<vislib::sys::File*>(data);
    megamol::console::CoreHandle hParam;

    if (!::mmcGetParameterA(hCore, str, hParam)) {
        vislib::sys::WriteFormattedLineToFile(*pfile,
            "# Failed to get handle for parameter %s\n", str);
        return;
    }

    unsigned int len = 0;
    unsigned char *buf = NULL;
    ::mmcGetParameterTypeDescription(hParam, NULL, &len);
    buf = new unsigned char[len];
    ::mmcGetParameterTypeDescription(hParam, buf, &len);
    if ((len >= 6) && (::memcmp(buf, "MMBUTN", 6) == 0)) {
        pfile->Write("# ", 2);
    }

    if (useRelativeFileNames && (len >= 6) && (
               (::memcmp(buf, "MMSTRW", 6) == 0)
            || (::memcmp(buf, "MMSTRA", 6) == 0)
            || (::memcmp(buf, "MMFILW", 6) == 0)
            || (::memcmp(buf, "MMFILA", 6) == 0) )) {
        ARY_SAFE_DELETE(buf);
        // fetch value
        vislib::TString v = ::mmcGetParameterValue(hParam);

        if (vislib::sys::Path::IsAbsolute(v)) {
            // value seems like a legal path
            vislib::TString paramFileDir
                = vislib::sys::Path::GetDirectoryName(parameterFile);

            // make value relative to param file
            v = relativePathTo(v, paramFileDir);
            vislib::sys::WriteFormattedLineToFile(*pfile,
                "%s=%s\n", str, vislib::StringA(v).PeekBuffer());
            return;

        }
    }

    delete[] buf;

    vislib::sys::WriteFormattedLineToFile(*pfile,
        "%s=%s\n", str, ::mmcGetParameterValueA(hParam));
}

}


/**
 * Writes all values of all parameters to the parameter file.
 */
void writeParameterFile(void) {
    vislib::sys::BufferedFile pfile;
    if (!pfile.Open(parameterFile, vislib::sys::File::WRITE_ONLY,
            vislib::sys::File::SHARE_READ,
            vislib::sys::File::CREATE_OVERWRITE)) {
        vislib::sys::Log::DefaultLog.WriteMsg(
            vislib::sys::Log::LEVEL_ERROR,
            "Unable to create parameter file.");
        return;
    } else {
        vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_INFO,
            "Writing parameter file \"%s\"",
            vislib::StringA(parameterFile).PeekBuffer());
    }

    vislib::sys::WriteFormattedLineToFile(pfile,
        "#\n# Parameter file created by MegaMol%s Console\n#\n",
        ASCIIStringTM());

    ::mmcEnumParametersA(hCore, writeParameterFileParameter,
        static_cast<void*>(&pfile));

    pfile.Close();
}


/**
 * Sets all values based on the content of the parameter file.
 */
void readParameterFile(void) {
    vislib::sys::BufferedFile pfile;
    if (!pfile.Open(parameterFile, vislib::sys::File::READ_ONLY,
            vislib::sys::File::SHARE_READ,
            vislib::sys::File::OPEN_ONLY)) {
        vislib::sys::Log::DefaultLog.WriteMsg(
            vislib::sys::Log::LEVEL_WARN,
            "Unable to open parameter file.");
        return;
    } else {
        vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_INFO,
            "Reading parameter file \"%s\"",
            vislib::StringA(parameterFile).PeekBuffer());
    }

    while (!pfile.IsEOF()) {
        vislib::StringA line = vislib::sys::ReadLineFromFileA(pfile);
        line.TrimSpaces();
        if (line.IsEmpty()) continue;
        if (line[0] == '#') continue;
        vislib::StringA::Size pos = line.Find('=');
        vislib::StringA name = line.Substring(0, pos);
        vislib::StringA value = line.Substring(pos + 1);
        {
            ::megamol::console::CoreHandle hParam;
            if (::mmcGetParameterA(hCore, name, hParam)) {

                if (useRelativeFileNames && !value.IsEmpty()) {
                    // resolve potential relative paths :-/
                    vislib::StringA fullPath = vislib::sys::Path::Resolve(value, 
                        vislib::StringA(vislib::sys::Path::GetDirectoryName(parameterFile)));
                    if (vislib::sys::File::Exists(fullPath)) {
                        value = fullPath;
                    }
                }

                ::mmcSetParameterValueA(hParam, value);
            } else {
                vislib::sys::Log::DefaultLog.WriteMsg(
                    vislib::sys::Log::LEVEL_ERROR,
                    "Unable to get handle for parameter \"%s\"\n",
                    name.PeekBuffer());
            }
        }
    }

    pfile.Close();
}


/*
 * setWindowPosition
 */
void setWindowPosition(const vislib::TString& id, const vislib::TString& val) {
    vislib::SingleLinkedList<vislib::SmartPtr<megamol::console::Window>
        >::Iterator iter = megamol::console::WindowManager::Instance()
        ->GetIterator();
    bool fullscreen = false;
    bool vp[4] = { false, false, false, false };
    int vv[4] = { 0, 0, 1, 1 };
    int vi = -1;

    while (iter.HasNext()) {
        vislib::SmartPtr<megamol::console::Window> &wnd = iter.Next();
        unsigned int wndIDsize;
        vislib::TString wndID;
        ::mmcGetInstanceID(wnd->HView(), NULL, &wndIDsize);
        ::mmcGetInstanceID(wnd->HView(), wndID.AllocateBuffer(wndIDsize + 1),
            &wndIDsize);
        if (wndID.Equals(id)) {
            vislib::TString v(val);
            v.TrimSpaces();
            while (!v.IsEmpty()) {
                vi = -1;
                if ((v[0] == _T('F')) || (v[0] == _T('f'))) {
                    fullscreen = true;
                } else if ((v[0] == _T('X')) || (v[0] == _T('x'))) {
                    vi = 0;
                } else if ((v[0] == _T('Y')) || (v[0] == _T('y'))) {
                    vi = 1;
                } else if ((v[0] == _T('W')) || (v[0] == _T('w'))) {
                    vi = 2;
                } else if ((v[0] == _T('H')) || (v[0] == _T('h'))) {
                    vi = 3;
                } else {
                    vislib::sys::Log::DefaultLog.WriteMsg(
                        vislib::sys::Log::LEVEL_WARN,
                        "Unexpected character %s in window position definition.\n",
                        vislib::StringA(v[0], 1).PeekBuffer());
                    break;
                }
                v = v.Substring(1);
                v.TrimSpaces();
                if (vi >= 0) {
                    int cp = 0;
                    int val = 0;
                    bool positive = true;
                    if (v[cp] == _T('+')) {
                        positive  = true;
                        cp++;
                    } else if (v[cp] == _T('-')) {
                        positive  = false;
                        cp++;
                    }
                    for (;(cp < v.Length()) && (v[cp] >= _T('0')) && (v[cp] <= _T('9')); cp++) {
                        val *= 10;
                        val += (v[cp] - _T('0'));
                    }
                    if (!positive) val = -val;
                    if ((vi > 1) && (v <= 0)) {
                        val = 1;
                        vislib::sys::Log::DefaultLog.WriteMsg(
                            vislib::sys::Log::LEVEL_WARN,
                            "Negative (or zero) size clamped to one");
                    }
                    v = v.Substring(cp);
                    vp[vi] = true;
                    vv[vi] = val;
                }
            }

            if (vp[2] && vp[3]) {
                // set size
                mmvSetWindowSize(wnd->HWnd(), 
                    static_cast<unsigned int>(vv[2]), 
                    static_cast<unsigned int>(vv[3]));

            } else if (vp[2] || vp[3]) {
                vislib::sys::Log::DefaultLog.WriteMsg(
                    vislib::sys::Log::LEVEL_WARN,
                    "Cannot set incomplete size");
            }

            if (vp[0] && vp[1]) {
                // set pos
                mmvSetWindowPosition(wnd->HWnd(),
                    vv[0], vv[1]);
            } else if (vp[0] || vp[1]) {
                vislib::sys::Log::DefaultLog.WriteMsg(
                    vislib::sys::Log::LEVEL_WARN,
                    "Cannot set incomplete position");
            }

            if (fullscreen) {
                // set fullscreen
                mmvSetWindowFullscreen(wnd->HWnd());
            }

        }
    }
}


extern "C" {

/*
 * [DEBUG] Test relevance of a parameter for a given view handle. This is a
 * test function ment to test the relevance checks in preperation for a GUI.
 *
 * @param str The full name of a parameter.
 * @param data The view handle.
 */
void MEGAMOLCORE_CALLBACK testParameterRelevance(const char* str, void *data) {
    megamol::console::CoreHandle hParam;
    void *hView = data;
    if (!::mmcGetParameterA(hCore, str, hParam)) {
        vislib::sys::Log::DefaultLog.WriteMsg(
            vislib::sys::Log::LEVEL_ERROR,
            "Unable to get handle for parameter \"%s\"\n",
            str);
        return;
    }

    vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_INFO + 200,
        "Parameter \"%s\" is%s relevant.\n", str,
        (::mmcIsParameterRelevant(hView, hParam) ? "" : " *NOT*"));
}

}


/**
 * The menu command callback method
 *
 * @param wnd The window calling
 * @param params The call parameters (value of the clicked menu item)
 */
void menuCommandCallback(void *wnd, int *params) {
    switch(*params) {
        case 1: // write parameter file
            if (parameterFile.IsEmpty()) break;
            writeParameterFile();
            break;
        case 2: // read parameter file
            if (parameterFile.IsEmpty()) break;
            readParameterFile();
            break;
#ifdef WITH_TWEAKBAR
        case 3:
            static_cast<megamol::console::Window *>(wnd)->ActivateGUI();
            break;
        case 4:
            static_cast<megamol::console::Window *>(wnd)->DeactivateGUI();
            break;
#endif /* WITH_TWEAKBAR */
        default:
            vislib::sys::Log::DefaultLog.WriteMsg(
                vislib::sys::Log::LEVEL_ERROR,
                "Menu command \"%d\" not implemented.", *params);
            break;
    }
}


/**
 * Callback called by the viewer if the termination of the whole application
 * is requested.
 */
void viewerRequestsAppExitCallback(void *userData, void *params) {
    terminationRequest = true;
}


/**
 * Runs the application in 'help' mode providing online help information.
 *
 * @param parser The main parser object.
 *
 * @return The return value of the application.
 */
int runHelp(megamol::console::utility::CmdLineParser *&parser) {
    VLSTACKTRACE("runHelp", __FILE__, __LINE__);

    printf("\nVersion:\n");
    megamol::console::utility::AboutInfo::PrintVersionInfo();

    // show help
    parser->PrintHelp();

    printf("\nFor additional information consult the manual or the webpage:\n");
    printf("    http://www.vis.uni-stuttgart.de/~grottel/megamol\n\n");

    return 0;
}


#ifndef _WIN32
void (* oldSignl)(int);
#endif /* !_WIN32 */


/**
 * Signal handler for Ctrl+C to change from abort to clean program shutdown.
 *
 * @param The signal to be handled.
 */
void signalCtrlC(int) {
    fprintf(stderr, "SIGINT received. Program termination requested.\n");
    terminationRequest = true;
#ifndef _WIN32
    signal(SIGINT, oldSignl);
#endif /* !_WIN32 */
}


extern "C" {

/**
 * Searching for file name parameters
 *
 * @param str The parameter name
 * @param data not used
 */
void MEGAMOLCORE_CALLBACK fixFileNameEnum(const char *str, void *data) {
    VLSTACKTRACE("fixFileName", __FILE__, __LINE__);
    vislib::StringA n(str);
    n.ToLowerCase();
    if (n.EndsWith("filename")) {
        megamol::console::CoreHandle hParam;

        if (!::mmcGetParameterA(hCore, str, hParam)) {
            fprintf(stderr, "Failed to get handle for parameter %s\n", str);
            return;
        }

        vislib::StringA fn(::mmcGetParameterValueA(hParam));

        if (!vislib::sys::File::Exists(fn)) {
            // we need to search for a better file

            // try 1: remove last char:
            vislib::StringA tfn = fn.Substring(0, fn.Length() - 1);
            if (vislib::sys::File::Exists(tfn)) {
                ::mmcSetParameterValueA(hParam, tfn);
                return;
            }

        }

    }
}

}


/**
 * Fixing the file names
 */
static void fixFileName(void) {
    VLSTACKTRACE("fixFileName", __FILE__, __LINE__);
    ::mmcEnumParametersA(hCore, fixFileNameEnum, NULL);
}


/**
 * runNormal
 *
 * @param parser The main parser object.
 *
 * @return The return value of the application.
 */
int runNormal(megamol::console::utility::CmdLineParser *&parser) {
    VLSTACKTRACE("runNormal", __FILE__, __LINE__);
    using vislib::sys::Log;
    MMC_USING_VERIFY;
    bool loadViewer = parser->IsViewerForced();
    vislib::SingleLinkedList<vislib::TString> projects;
    vislib::SingleLinkedList<vislib::TString> quickstarts;
    vislib::TMultiSz insts;
    vislib::Map<vislib::TString, vislib::TString> paramValues;
    vislib::TMultiSz winPoss;
    bool initParameterFile = false;
    bool initOnlyParameterFile = false;
    bool setVSync = false;
    bool vSyncOff = false;
    int retval = 0;
    bool showGUI = false;
    bool hideGUI = false;
    bool useQuadBuffers = false;
    vislib::SingleLinkedList<vislib::StringA> hotFixes;

#ifndef _WIN32
    oldSignl =
#endif /* !_WIN32 */
    signal(SIGINT, signalCtrlC);

    parameterFile = vislib::sys::Path::Resolve(parser->ParameterFile());
    initParameterFile = parser->InitParameterFile();
    initOnlyParameterFile = parser->InitOnlyParameterFile();
    setVSync = parser->SetVSync();
    vSyncOff = parser->SetVSyncOff();
    showGUI = parser->ShowGUI();
    hideGUI = parser->HideGUI();
    useQuadBuffers = parser->RequestOpenGLQuadBuffer();
    parser->GetHotFixes(hotFixes);
    
    if (hotFixes.Contains("relFileNames")) {
        useRelativeFileNames = true;
    }

    // run the application!
#ifdef _WIN32
    //vislib::sys::Console::SetTitle(L"MegaMol�");
    //vislib::sys::Console::SetIcon(100);
#endif /* _WIN32 */

    // Create an instance
    mmcErrorCode ccrv = ::mmcCreateCore(hCore);
    if (ccrv != MMC_ERR_NO_ERROR) {
        fprintf(stderr, "Unable to create core instance handle:\n");
        switch (ccrv) {
            case MMC_ERR_MEMORY:
                fprintf(stderr, "\tFailed to allocate instance memory.\n");
                break;
            case MMC_ERR_HANDLE:
                fprintf(stderr, "\tInvalid handle pointer encountered.\n");
                break;
            case MMC_ERR_LICENSING:
                fprintf(stderr, "\tNo valid license found.\n");
                break;
            case MMC_ERR_UNKNOWN:
                fprintf(stderr, "\tUnknown internal error encountered.\n");
                break;
            default:
                fprintf(stderr, "\tUnexpected error code returned.\n");
                break;
        }
        return -21;
    }

    try { // initialise core
        //MMC_VERIFY_THROW(::mmcSetInitialisationValue(hCore, // is now deprecated
        //    MMC_INITVAL_INCOMINGLOG, MMC_TYPE_VOIDP, 
        //    static_cast<void*>(&Log::DefaultLog)));
        // HAZARD!!! Cross-Heap-Allocation Problem
        // instead inquire the core log
        vislib::sys::Log *corelog = NULL;
        MMC_VERIFY_THROW(::mmcSetInitialisationValue(hCore, MMC_INITVAL_CORELOG, MMC_TYPE_VOIDP, static_cast<void*>(&corelog)));
        if (corelog != NULL) {
            Log::DefaultLog.SetEchoTarget(new vislib::sys::Log::RedirectTarget(corelog, Log::LEVEL_ALL));
            Log::DefaultLog.EchoOfflineMessages(true);
        }

        MMC_VERIFY_THROW(::mmcSetInitialisationValue(hCore,
            MMC_INITVAL_LOGECHOFUNC, MMC_TYPE_VOIDP,
            function_cast<void*>(writeLogEchoToConsole)));

        if (parser->IsConfigFileSpecified()) {
            MMC_VERIFY_THROW(::mmcSetInitialisationValue(hCore, 
                MMC_INITVAL_CFGFILE, MMC_TYPE_TSTR, 
                parser->ConfigFile()));
        }
        vislib::TMultiSz configSets = parser->ConfigSets();
        if (configSets.Count() > 0) {
            unsigned int cnt = static_cast<unsigned int>(configSets.Count());
            for (unsigned int i = 0; i < cnt; i++) {
                MMC_VERIFY_THROW(::mmcSetInitialisationValue(hCore, 
                    MMC_INITVAL_CFGSET, MMC_TYPE_TSTR, 
                    configSets[i].PeekBuffer()));
            }
        }
        if (parser->IsLogFileSpecified()) {
            MMC_VERIFY_THROW(::mmcSetInitialisationValue(hCore, 
                MMC_INITVAL_LOGFILE, MMC_TYPE_TSTR, 
                parser->LogFile()));
        }
        if (parser->IsLogLevelSpecified()) {
            unsigned int level = parser->LogLevel();
            MMC_VERIFY_THROW(::mmcSetInitialisationValue(hCore, 
                MMC_INITVAL_LOGLEVEL, MMC_TYPE_UINT32, 
                &level));
        }
        if (parser->IsLogEchoLevelSpecified()) {
            unsigned int level = parser->LogEchoLevel();
            MMC_VERIFY_THROW(::mmcSetInitialisationValue(hCore, 
                MMC_INITVAL_LOGECHOLEVEL, MMC_TYPE_UINT32, 
                &level));
        }

        parser->GetProjectFiles(projects);

        insts = parser->Instantiations();

        parser->GetParameterValueOptions(paramValues);

        winPoss = parser->WindowPositions();

        MMC_VERIFY_THROW(::mmcInitialiseCoreInstance(hCore));

        if (parser->HasQuickstartRegistrations()) {
            vislib::SingleLinkedList<vislib::TString> regs;
            parser->GetQuickstartRegistrations(regs);
            vislib::SingleLinkedList<vislib::TString>::Iterator iter = regs.GetIterator();
            while (iter.HasNext()) {
                vislib::Array<vislib::TString> entries(vislib::TStringTokeniser::Split(iter.Next(), _T(";"), true));
                for (SIZE_T i = 0; i < entries.Count(); i++) {
                    vislib::TString ext;
                    bool unreg = false;
                    bool over = true;
                    vislib::TString::Size pos = entries[i].Find(_T('|'));

                    if (pos != vislib::TString::INVALID_POS) {
                        vislib::Array<vislib::TString> opts(vislib::TStringTokeniser::Split(entries[i], _T('|'), true));
                        ASSERT(opts.Count() > 0);
                        ext = opts[0];
                        for (SIZE_T j = 1; j < opts.Count(); j++) {
                            if (opts[j].Equals(_T("unregister"), false)) {
                                unreg = true;
                            } else if (opts[j].Equals(_T("keepothers"), false)) {
                                over = false;
                            } else {
                                Log::DefaultLog.WriteWarn("Quickstart registration option \"%s\" ignored",
                                    vislib::StringA(opts[j]).PeekBuffer());
                            }
                        }
                    } else {
                        ext = entries[i];
                    }

                    ::mmcQuickstartRegistry(hCore, applicationExecutablePath,
                        _T("-q $(FILENAME)"), ext, unreg, over);

                }
            }
        }

        parser->GetQuickstarts(quickstarts);

        SAFE_DELETE(parser);

    } catch(vislib::Exception ex) {
        fprintf(stderr, "Unable to initialise core instance: %s [%s:%i]\n",
            ex.GetMsgA(), ex.GetFile(), ex.GetLine());
        return -22;
    } catch(...) {
        fprintf(stderr, "Unable to initialise core instance.\n");
        return -22;
    }

    if (loadViewer && !forceViewerLib()) {
        return -23;
    }

    if (!setVSync) {
        // not set by command line parameter
        // ask core for vsync configuration
        mmcValueType type;
        const void *cfgv = ::mmcGetConfigurationValueA(hCore,
            MMC_CFGID_VARIABLE, "vsync", &type);
        switch (type) {
            case MMC_TYPE_BOOL:
                setVSync = true;
                vSyncOff = !(*static_cast<const bool*>(cfgv));
                break;
            case MMC_TYPE_CSTR:
                try {
                    vSyncOff = !vislib::CharTraitsA::ParseBool(
                        static_cast<const char *>(cfgv));
                    setVSync = true;
                } catch(...) {
                }
                break;
            case MMC_TYPE_WSTR:
                try {
                    vSyncOff = !vislib::CharTraitsW::ParseBool(
                        static_cast<const wchar_t *>(cfgv));
                    setVSync = true;
                } catch(...) {
                }
                break;
#ifndef _WIN32
            default:
                // intentionally empty
                break;
#endif /* !_WIN32 */
        }
    }

    if (!showGUI && !hideGUI) {
        // not set by command line parameter
        // ask core for gui configuration
        mmcValueType type;
        const void *cfgv = ::mmcGetConfigurationValueA(hCore,
            MMC_CFGID_VARIABLE, "consolegui", &type);
        switch (type) {
            case MMC_TYPE_BOOL:
                if (*static_cast<const bool*>(cfgv)) {
                    showGUI = true;
                } else {
                    hideGUI = true;
                }
                break;
            case MMC_TYPE_CSTR:
                try {
                    bool b = vislib::CharTraitsA::ParseBool(
                        static_cast<const char *>(cfgv));
                    if (b) {
                        showGUI = true;
                    } else {
                        hideGUI = true;
                    }
                } catch(...) {
                }
                break;
            case MMC_TYPE_WSTR:
                try {
                    bool b = vislib::CharTraitsW::ParseBool(
                        static_cast<const wchar_t *>(cfgv));
                    if (b) {
                        showGUI = true;
                    } else {
                        hideGUI = true;
                    }
                } catch(...) {
                }
                break;
#ifndef _WIN32
            default:
                // intentionally empty
                break;
#endif /* !_WIN32 */
        }
    }

    if (showGUI && hideGUI) {
        showGUI = false;
        hideGUI = false;
    }

    // Load Projects
    vislib::SingleLinkedList<vislib::TString>::Iterator
        prjIter = projects.GetIterator();
    while (prjIter.HasNext()) {
        ::mmcLoadProject(hCore, prjIter.Next());
    }

    // try to create all requested instances
    ASSERT((insts.Count() % 2) == 0);
    SIZE_T instsCnt = insts.Count() / 2;
    for (SIZE_T i = 0; i < instsCnt; i++) {
        ::mmcRequestInstance(hCore, insts[i * 2], insts[i * 2 + 1]);
    }

    vislib::SingleLinkedList<vislib::TString>::Iterator qsi = quickstarts.GetIterator();
    while (qsi.HasNext()) {
        ::mmcQuickstart(hCore, qsi.Next());
    }

    if (::mmcHasPendingJobInstantiationRequests(hCore)) {
        vislib::SmartPtr<megamol::console::CoreHandle> jobHandle;

        while (::mmcHasPendingJobInstantiationRequests(hCore)) {
            jobHandle = new megamol::console::CoreHandle();

            if (!::mmcInstantiatePendingJob(hCore,
                    jobHandle->operator void*())) {
                vislib::sys::Log::DefaultLog.WriteMsg(
                    vislib::sys::Log::LEVEL_ERROR,
                    "Unable to instantiate requested job.");
                retval = -28;
                continue;
            }

            megamol::console::JobManager::Instance()->Add(jobHandle);
        }
    }

    if (::mmcHasPendingViewInstantiationRequests(hCore)) {
        if (!forceViewerLib()) {
            return -24;
        }

        unsigned int conviewhints = MMV_VIEWHINT_NONE;
        if (!useQuadBuffers) {
            // ::mmcGetConfigurationValueA(
        }
        if (useQuadBuffers) {
            conviewhints |= MMV_VIEWHINT_QUADBUFFER;
        }
        if (!::mmvCreateViewerHandle(hView, conviewhints)) {
            vislib::sys::Log::DefaultLog.WriteMsg(
                vislib::sys::Log::LEVEL_ERROR,
                "Unable to initialise a viewer instance.");
            return -25;
        }

        // Initialise views
        while (::mmcHasPendingViewInstantiationRequests(hCore)) {
            vislib::SmartPtr<megamol::console::Window> win
                = new megamol::console::Window();

            if (!::mmvCreateWindow(hView, win->HWnd())) {
                vislib::sys::Log::DefaultLog.WriteMsg(
                    vislib::sys::Log::LEVEL_ERROR,
                    "Unable to create rendering window.");
                retval = -28;
                continue;
            }
            ::mmvSetUserData(win->HWnd(),
                static_cast<void*>(win.operator->()));
            if (!::mmcInstantiatePendingView(hCore, win->HView())) {
                ::mmvSetUserData(win->HWnd(), NULL);
                ::mmvDisposeHandle(win->HWnd());
                vislib::sys::Log::DefaultLog.WriteMsg(
                    vislib::sys::Log::LEVEL_ERROR,
                    "Unable to instantiate requested view.");
                retval = -27;
                continue;
            }

            if (setVSync) {
                ::mmvSetWindowHints(win->HWnd(), MMV_WINHINT_VSYNC,
                    vSyncOff ? MMV_WINHINT_NONE : MMV_WINHINT_VSYNC);
            }

            if (::mmvSupportContextMenu(win->HWnd())) {
                ::mmvInstallContextMenu(win->HWnd());

#ifdef WITH_TWEAKBAR
                /* TODO: Move GUI */
                /*if (::mmvSupportParameterGUI(win->HWnd()))*/ {
                    ::mmvInstallContextMenuCommandA(win->HWnd(), "Activate GUI", 3);
                    ::mmvInstallContextMenuCommandA(win->HWnd(), "Deactivate GUI", 4);
                    if (hideGUI) {
                        win->DeactivateGUI();
                    }
                } /* else {
                    if (hideGUI || showGUI) {
                        vislib::sys::Log::DefaultLog.WriteMsg(
                            vislib::sys::Log::LEVEL_WARN,
                            "Parameter GUI is not supported by the viewer");
                    }
                } */
#endif /* WITH_TWEAKBAR */

                if (!parameterFile.IsEmpty()) {
                    ::mmvInstallContextMenuCommandA(win->HWnd(),
                        "Write Parameter File", 1);
                    ::mmvInstallContextMenuCommandA(win->HWnd(),
                        "Read Parameter File", 2);
                }
            }
            if (!parameterFile.IsEmpty()) {
                win->RegisterHotKeyAction(vislib::sys::KeyCode(vislib::sys::KeyCode::KEY_MOD_ALT | 'p'),
                    new megamol::console::HotKeyCallback(::readParameterFile), "ReadParameterFile");
                win->RegisterHotKeyAction(vislib::sys::KeyCode(
                    vislib::sys::KeyCode::KEY_MOD_ALT | vislib::sys::KeyCode::KEY_MOD_SHIFT | 'P'),
                    new megamol::console::HotKeyCallback(::writeParameterFile), "WriteParameterFile");
            }

            // register callbacks
            ::mmcRegisterViewCloseRequestFunction(win->HView(),
                megamol::console::Window::CloseRequestCallback,
                static_cast<void*>(win.operator->()));
            ::mmvRegisterWindowCallback(win->HWnd(), MMV_WINCB_CLOSE,
                megamol::console::Window::CloseCallback);
            ::mmvRegisterWindowCallback(win->HWnd(), MMV_WINCB_RENDER,
                megamol::console::Window::RenderCallback);
            ::mmvRegisterWindowCallback(win->HWnd(), MMV_WINCB_RESIZE,
                function_cast<mmvCallback>(function_cast<void*>(
                megamol::console::Window::ResizeCallback)));
            ::mmvRegisterWindowCallback(win->HWnd(), MMV_WINCB_KEY,
                function_cast<mmvCallback>(function_cast<void*>(
                megamol::console::Window::KeyCallback)));
            ::mmvRegisterWindowCallback(win->HWnd(), MMV_WINCB_MOUSEBUTTON,
                function_cast<mmvCallback>(function_cast<void*>(
                megamol::console::Window::MouseButtonCallback)));
            ::mmvRegisterWindowCallback(win->HWnd(), MMV_WINCB_MOUSEMOVE,
                function_cast<mmvCallback>(function_cast<void*>(
                megamol::console::Window::MouseMoveCallback)));
            ::mmvRegisterWindowCallback(win->HWnd(), MMV_WINCB_COMMAND,
                function_cast<mmvCallback>(function_cast<void*>(
                menuCommandCallback)));
            ::mmvRegisterWindowCallback(win->HWnd(), MMV_WINCB_APPEXIT,
                function_cast<mmvCallback>(function_cast<void*>(
                viewerRequestsAppExitCallback)));
            ::mmvRegisterWindowCallback(win->HWnd(), MMV_WINCB_UPDATEFREEZE,
                function_cast<mmvCallback>(function_cast<void*>(
                megamol::console::Window::UpdateFreezeCallback)));

            megamol::console::WindowManager::Instance()->Add(win);

            unsigned int strLen;
            vislib::TString str;
            ::mmcGetInstanceID(win->HView(), NULL, &strLen);
            ::mmcGetInstanceID(win->HView(), str.AllocateBuffer(strLen), &strLen);
            vislib::TString::Size p = str.Find(_T("::"));
            if (p != vislib::StringA::INVALID_POS) {
                str.Truncate(p);
            }
            ::mmvSetWindowTitle(win->HWnd(), str.PeekBuffer());
            vislib::sys::Log::DefaultLog.WriteMsg(
                vislib::sys::Log::LEVEL_INFO + 50,
                "Testing Relevance of Parameters for \"%s\":",
                vislib::StringA(str).PeekBuffer());
            ::mmcEnumParametersA(hCore, testParameterRelevance, win->HView());
            int wndX, wndY, wndW, wndH;
            bool wndND;
            if (::mmcDesiredViewWindowConfig(win->HView(),
                    &wndX, &wndY, &wndW, &wndH, &wndND)) {
                if (wndND) {
                    unsigned int flags = MMV_WINHINT_NODECORATIONS | MMV_WINHINT_STAYONTOP;
                    if (!hotFixes.Contains("DontHideCursor")) {
                        flags |= MMV_WINHINT_HIDECURSOR;
                    }
                    ::mmvSetWindowHints(win->HWnd(), flags, flags);

                    // TODO: Support parameters
                    ::mmvSetWindowHints(win->HWnd(), MMV_WINHINT_PRESENTATION, MMV_WINHINT_PRESENTATION);

                }
                if ((wndX != INT_MIN) && (wndY != INT_MIN)) {
                    ::mmvSetWindowPosition(win->HWnd(), wndX, wndY);
                }
                if ((wndW != INT_MIN) && (wndH != INT_MIN)) {
                    ::mmvSetWindowSize(win->HWnd(), wndW, wndH);
                }
            }

            win->RegisterHotKeys(hCore);
            if (hotFixes.Contains("EnableFileNameFix")) {
                win->RegisterHotKeyAction(
                    vislib::sys::KeyCode(
                    /*(WORD)vislib::sys::KeyCode::KEY_MOD_CTRL
                    | (WORD)vislib::sys::KeyCode::KEY_MOD_SHIFT
                    | */(WORD)'f'), new megamol::console::HotKeyCallback(::fixFileName),
                    "FileNameFix");
            }
#ifdef WITH_TWEAKBAR
            win->InitGUI(hCore);
            if (showGUI) {
                win->ActivateGUI();
            }
#endif /* WITH_TWEAKBAR */
        }

        if (megamol::console::WindowManager::Instance()->Count() == 0) {
            vislib::sys::Log::DefaultLog.WriteMsg(
                vislib::sys::Log::LEVEL_ERROR,
                "Unable to instantiate any of the requested views.\n");
            return -26;
        }
    }

    // set parameter values
    vislib::Map<vislib::TString, vislib::TString>::Iterator
        pvIter = paramValues.GetIterator();
    while (pvIter.HasNext()) {
        vislib::Map<vislib::TString, vislib::TString>::ElementPair&
            pv = pvIter.Next();
        ::megamol::console::CoreHandle hParam;
        if (!::mmcGetParameter(hCore, pv.Key(), hParam)) {
            vislib::sys::Log::DefaultLog.WriteMsg(
                vislib::sys::Log::LEVEL_ERROR,
                "Unable to get handle for parameter \"%s\".\n",
                vislib::StringA(pv.Key()).PeekBuffer());
            continue;
        }
        ::mmcSetParameterValue(hParam, pv.Value());
    }

    if (!parameterFile.IsEmpty()) {
        if (initParameterFile) {
            writeParameterFile();
        } else if (vislib::sys::File::Exists(parameterFile)) {
            readParameterFile();
        } else {
            vislib::sys::Log::DefaultLog.WriteMsg(
                vislib::sys::Log::LEVEL_INFO,
                "Parameter file \"%s\" does not exist",
                vislib::StringA(parameterFile).PeekBuffer());
        }
    }

    megamol::console::JobManager::Instance()->StartJobs();

    for (SIZE_T i = 0; i < winPoss.Count(); i += 2) {
        setWindowPosition(winPoss[i], winPoss[i + 1]);
    }

    // Note: This frontend is not capable of createing new (view/job)instances
    // after this point is reached! (ATM)

    // main message loop
    bool running = !initOnlyParameterFile;
    while (running) {
        running = false; // will be reactivated if one element requests it

        // view message loop
        if (hView.IsValid()) {
            if (::mmvProcessEvents(hView)) {
                running = true;
            }
        }

        // check if we still got running jobs.
        if (megamol::console::JobManager::Instance()->CheckJobs()) {
            if (terminationRequest) {
                // terminate all job
                megamol::console::JobManager::Instance()->TerminateJobs();
            }
            running = true;
        }

        if (terminationRequest) {
            megamol::console::WindowManager::Instance()->MarkAllForClosure();
        }

        // remove closed windows
        if (megamol::console::Window::HasClosed()) {
            megamol::console::WindowManager::Instance()->Cleanup();
        }

    }

    megamol::console::JobManager::Instance()->TerminateJobs();
    while (megamol::console::JobManager::Instance()->CheckJobs()) {
        vislib::sys::Thread::Sleep(1);
    }
    megamol::console::WindowManager::Instance()->CloseAll();
    if (hView.IsValid()) {
        hView.DestroyHandle();
    }
    if (megamol::viewer::mmvIsLibraryLoaded()) {
        Log::DefaultLog.WriteMsg(Log::LEVEL_INFO + 200, "unloading Viewer");
        megamol::viewer::mmvUnloadLibrary();
    }
    //dummyEchoTarget = new writeLogEchoToConsoleEchoTarget();
    Log::DefaultLog.SetEchoTarget(NULL);
    hCore.DestroyHandle();

    return retval;
}


/**
 * The applications main entry point
 *
 * @param argc The number of arguments.
 * @param argv The arguments.
 *
 * @return The return value of the application.
 */
#ifdef _WIN32
int _tmain(int argc, _TCHAR* argv[]) {
#if (defined(DEBUG) || defined(_DEBUG)) && defined(_CRTDBG_MAP_ALLOC)
    _CrtSetDbgFlag(
        _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF// | _CRTDBG_CHECK_ALWAYS_DF
        | _CRTDBG_CHECK_CRT_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_CHECK_EVERY_128_DF);
#if defined(MY_CRTDBG_BREAK_AT_ALLOC)
    _CrtSetBreakAlloc(MY_CRTDBG_BREAK_AT_ALLOC);
#endif /*  */
#endif /* DEBUG || _DEBUG */
#else /* _WIN32 */
int main(int argc, char* argv[]) {
#endif /* _WIN32 */
    int retVal = 0;
    megamol::console::utility::CmdLineParser *parser = NULL;

    vislib::Trace::GetInstance().SetLevel(vislib::Trace::LEVEL_VL);

    vislib::sys::ThreadSafeStackTrace::Initialise();
    {
        vislib::SmartPtr<vislib::StackTrace> manager
            = vislib::sys::ThreadSafeStackTrace::Manager();
        ::mmcSetInitialisationValue(NULL, MMC_INITVAL_VISLIB_STACKTRACEMANAGER,
            MMC_TYPE_VOIDP, static_cast<void*>(&manager));
    }
    VLSTACKTRACE("main", __FILE__, __LINE__);
    ::terminationRequest = false;
    ::echoedImportant = false;

    applicationExecutablePath = argv[0];

    parameterFile.Clear();

    try {
        vislib::sys::TCmdLineProvider cmdline(argc, argv);

        vislib::sys::Log::DefaultLog.SetLogFileName(
            static_cast<const char*>(NULL), false);
        vislib::sys::Log::DefaultLog.SetLevel(vislib::sys::Log::LEVEL_ALL);
        vislib::sys::Log::DefaultLog.SetEchoLevel(
            vislib::sys::Log::LEVEL_NONE);
        vislib::sys::Log::DefaultLog.SetEchoTarget(NULL);

        megamol::console::utility::AboutInfo::LogGreeting();
        megamol::console::utility::AboutInfo::LogVersionInfo();
        { // startup information
            time_t nowtime;
            char buffer[1024];
            struct tm *now;
            time(&nowtime);
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
            struct tm nowdata;
            now = &nowdata;
            localtime_s(now, &nowtime);
#else /* defined(_WIN32) && (_MSC_VER >= 1400) */
            now = localtime(&nowtime);
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */
            strftime(buffer, 1024, "%#c", now);
            vislib::sys::Log::DefaultLog.WriteMsg(
                vislib::sys::Log::LEVEL_INFO, "Started %s\n", buffer);
        }

        parser = new megamol::console::utility::CmdLineParser();

        // Parse the command line
        retVal = parser->Parse(cmdline.ArgC(), cmdline.ArgV());

        if (parser->NoConColour()) {
            vislib::sys::Console::EnableColors(false);
        }

        if (retVal < 0) {
            // errors are present!
            megamol::console::utility::AboutInfo::PrintGreeting();
            parser->PrintErrorsAndWarnings(stderr);
            fprintf(stderr, "\nUse \"--help\" for usage information\n\n");

        } else {
            // ok or warnings!

            if (parser->UseCmdLineFile()) {
                const TCHAR *cmdlinefile = parser->CmdLineFile();
                if (cmdlinefile == NULL) {
                    vislib::sys::Log::DefaultLog.WriteMsg(
                        vislib::sys::Log::LEVEL_ERROR,
                        "Unable to retreive command line file name.\n");

                } else {
                    vislib::TString cmdlinefilename(cmdlinefile);
                    vislib::sys::BufferedFile file;
                    if (file.Open(cmdlinefilename, vislib::sys::File::READ_ONLY,
                            vislib::sys::File::SHARE_READ,
                            vislib::sys::File::OPEN_ONLY)) {

                        vislib::StringA newCmdLine
                            = vislib::sys::ReadLineFromFileA(file, 100000);

                        file.Close();

                        cmdline.CreateCmdLine(argv[0], A2T(newCmdLine));

                        retVal = parser->Parse(cmdline.ArgC(), cmdline.ArgV());

                        if (parser->NoConColour()) {
                            vislib::sys::Console::EnableColors(false);
                        }

                        if (retVal < 0) {
                            megamol::console::utility::AboutInfo::PrintGreeting();
                            parser->PrintErrorsAndWarnings(stderr);
                            fprintf(stderr, "\nUse \"--help\" for usage information\n\n");

                        } else {
                            vislib::sys::Log::DefaultLog.WriteMsg(
                                vislib::sys::Log::LEVEL_INFO,
                                "Read command line from \"%s\"\n",
                                vislib::StringA(cmdlinefilename).PeekBuffer());

                        }

                    } else {
                        vislib::sys::Log::DefaultLog.WriteMsg(
                            vislib::sys::Log::LEVEL_ERROR,
                            "Unable to open file \"%s\"\n",
                            vislib::StringA(cmdlinefilename).PeekBuffer());

                    }
                }

            }

            if (!parser->HideLogo() || parser->ShowVersionInfo()) {
                megamol::console::utility::AboutInfo::PrintGreeting();
            }

            if (parser->EchoCmdLine()) {
                // echo the command line
                vislib::StringA cmdlineecho("Called:");
                for (int i = 0; i < cmdline.ArgC(); i++) {
                    cmdlineecho.Append(" ");
                    cmdlineecho.Append(vislib::StringA(cmdline.ArgV()[i]));
                }
                
                vislib::sys::Log::DefaultLog.WriteMsg(
                    vislib::sys::Log::LEVEL_INFO, "%s\n",
                    cmdlineecho.PeekBuffer());
                if (vislib::sys::Log::DefaultLog.GetEchoLevel()
                        < vislib::sys::Log::LEVEL_INFO) {
                    printf("%s\n", cmdlineecho.PeekBuffer());
                }
            }

            if (parser->ShowVersionInfo()) {
                megamol::console::utility::AboutInfo::PrintVersionInfo();
            }

            if (retVal > 0) {
                parser->PrintErrorsAndWarnings(stderr);
            }

            // call the required 'run*' method
            if (retVal >= 0) {
                if (parser->WantHelp()) {
                    // online help
                    retVal = runHelp(parser);

                } else {
                    // normal operations
                    retVal = runNormal(parser);

                }
            }
        }

    } catch(vislib::Exception e) {
        vislib::StringA msg;
        msg.Format("Exception in (%s, %d): %s\n", e.GetFile(), e.GetLine(),
            e.GetMsgA());
        if (e.HasStack()) {
            msg.Append("\tStack Trace:\n");
            msg.Append(e.GetStack());
        }
        vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_ERROR,
            msg);
        retVal = -1;
    } catch(...) {
        vislib::sys::Log::DefaultLog.WriteMsg(vislib::sys::Log::LEVEL_ERROR,
            "Unknown Exception.");
        retVal = -1;
    }

    if (parser != NULL) {
        delete parser;
    }

    ::hView.DestroyHandle();
    ::hCore.DestroyHandle();

    // we're done
    if (::echoedImportant && (retVal == 0)) retVal = 1;
    printf("\n");
#if defined(_WIN32) && defined(_DEBUG) // VC Debugger Halt on Stop Crowbar
#pragma warning(disable: 4996)
    if ((retVal != 0) && (getenv("_ACP_LIB") != NULL)) system("pause");
#pragma warning(default: 4996)
#endif
    return retVal;
}
