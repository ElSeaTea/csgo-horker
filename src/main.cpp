#include "features/faim.h"
#include "features/fglow.h"
#include "features/fother.h"
#include "features/fvisual.h"
#include "sdk/cbaseentity.h"
#include "sdk/cglowobjectmanager.h"
#include "sdk/types.h"

#include "config.h"
#include "engine.h"
#include "globals.h"
#include "helper.h"
#include "hwctrl.h"
#include "offsets.h"
#include "process.h"

#include <chrono>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <signal.h>
#include <unistd.h>

#define LOG(X) std::cout << X << std::flush

bool shouldQuit = false;

void exitHandle(int)
{
    shouldQuit = true;
}

void connectSignals(struct sigaction &handle)
{
    handle.sa_handler = exitHandle;
    sigemptyset(&handle.sa_mask);
    handle.sa_flags = 0;
    sigaction(SIGINT, &handle, NULL);
    sigaction(SIGQUIT, &handle, NULL);
}

int main()
{

    if (getuid() != 0) {
        LOG("This program must be ran as root.\n");
        return 0;
    }

    char* displayName = getenv("DISPLAY");
    if (displayName) {
        printf("Display is: %s\n", displayName);
    } else {
        printf("Failed to find display!\n");
    }

    if (!Helper::Init()) {
        LOG("Failed to initialize input handling.\n");
        return 0;
    }
    
    struct sigaction ccHandle;
    connectSignals(ccHandle);
    
    Process proc(PROCESS_NAME);
    
    LOG("Waiting for process...");
    
    while (!proc.Attach() && !shouldQuit) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    LOG("Done.\nWaiting for client and engine library...");

    while (!shouldQuit) {
        proc.ParseModules();
        if (proc.HasModule(CLIENT_SO) && proc.HasModule(ENGINE_SO)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (shouldQuit) {
        return 0;
    }

    ReadConfig("config.ini");

    LOG("Done.\n");
    Signatures::Find(proc);
    Signatures::Print(proc);

    auto& eng = Engine::GetInstance();
    eng.SetProcessManager(&proc);
    eng.Update(true);
    // Feature handlers
    FAim faim(proc);
    FGlow fglow(proc);
    FOther fother(proc);
    FVisual fvisual(proc);

    bool useMouseButtonAim = false;
    if (Config::AimBot::ToggleKey.compare(0, 5, "Mouse") == 0) {
        useMouseButtonAim = true;
    }

    bool useMouseButtonGlow = false;
    if (Config::Glow::ToggleKey.compare(0, 5, "Mouse") == 0) {
        useMouseButtonGlow = true;
    }

    const int aimToggleKey = useMouseButtonAim ? 
        Helper::StringToMouseMask(Config::AimBot::ToggleKey) :
        Helper::StringToKeycode(Config::AimBot::ToggleKey);

    const int glowToggleKey = useMouseButtonGlow ? 
        Helper::StringToMouseMask(Config::Glow::ToggleKey) :
        Helper::StringToKeycode(Config::Glow::ToggleKey);

    bool aimRunning = false;
    bool glowRunning = false;

    bool runAim = true;
    bool runGlow = true;

    if (Config::Visual::Contrast != 0) {
        HWCtrl::SetContrast(Config::Visual::Contrast);
    }

    while (!shouldQuit) {
        if (!proc.IsValid()) {
            shouldQuit = true;
            LOG("Lost connection to process... Exiting.\n");
            break;
        }

        // ### BEGIN MENU HACKS ###
        
        // ### END MENU HACKS ###
        
        // ### BEGIN IN-GAME HACKS ###
        if (eng.IsConnected()) {
            faim.Start();
            aimRunning = true;
            fglow.Start();
            glowRunning = true;
            fother.Start();
            fvisual.Start();

            while (eng.IsConnected() && !shouldQuit) {
                eng.Update();

                bool shouldToggleAim = useMouseButtonAim ? Helper::IsMouseDown(aimToggleKey) :
                Helper::IsKeyDown(aimToggleKey);

                bool shouldToggleGlow = useMouseButtonGlow ? Helper::IsMouseDown(glowToggleKey) :
                Helper::IsKeyDown(glowToggleKey);

                runAim = shouldToggleAim ? !runAim : runAim;
                while (shouldToggleAim) {
                    shouldToggleAim = useMouseButtonAim ? Helper::IsMouseDown(aimToggleKey) :
                    Helper::IsKeyDown(aimToggleKey);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                runGlow = shouldToggleGlow ? !runGlow : runGlow;
                while (shouldToggleGlow) {
                    shouldToggleGlow = useMouseButtonGlow ? Helper::IsMouseDown(glowToggleKey) :
                    Helper::IsKeyDown(glowToggleKey);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }

                if (aimRunning && !runAim) {
                    faim.Stop();
                    aimRunning = false;
                } else if (!aimRunning && runAim) {
                    faim.Start();
                    aimRunning = true;
                }

                if (glowRunning && !runGlow) {
                    fglow.Stop();
                    glowRunning = false;
                } else if (!glowRunning && runGlow) {
                    fglow.Start();
                    glowRunning = true;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }

            faim.Stop();
            aimRunning = false;
            fglow.Stop();
            glowRunning = false;
            fother.Stop();
            fvisual.Stop();
        }
        // ### END IN-GAME HACKS ###
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    Helper::Finalize();
    if (Config::Visual::Contrast != 0) {
        HWCtrl::SetContrast(0);
    }
    return 0;
}
