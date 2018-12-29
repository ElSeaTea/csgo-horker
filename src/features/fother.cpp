#include "fother.h"
#include "../helper.h"
#include "../offsets.h"
#include "../config.h"
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <iostream>
#include <unistd.h>
void FOther::BHop(uintptr_t localPlayer)
{
    constexpr int FL_ONGROUND = (1<<0);

    int flags;
    if (m_mem.Read(localPlayer + Netvar::CBaseEntity::m_fFlags, &flags)) {
        if (Helper::IsKeyDown(Helper::KeysymToKeycode(XK_space))) {
            if (flags & FL_ONGROUND) {
                m_mem.Write(Offset::Client::ForceJump, 5, sizeof(uint8_t));
            } else {
                m_mem.Write(Offset::Client::ForceJump, 4, sizeof(uint8_t));
            }
        }
    }
}


void FOther::Run()
{
    uintptr_t localPlayer;
    Log("[FOther] Started");
    while (!ShouldStop()) {
        if (!m_mem.Read(Offset::Client::LocalPlayer, &localPlayer)) {
            LogWait("[FOther] Failed to read local player address.");
            continue;
        }
        if (Config::Other::BunnyHop) {
            BHop(localPlayer);
        }
        WaitMs(5);
    }
    Log("[FOther] Stopped");
}
