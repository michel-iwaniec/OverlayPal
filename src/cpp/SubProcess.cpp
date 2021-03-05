//
// This file is part of OverlayPal ( https://github.com/michel-iwaniec/OverlayPal )
// Copyright (c) 2021 Michel Iwaniec.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <stdexcept>

#include "SubProcess.h"

#ifdef _WIN32
int executeProcess(const std::string& exeFilename,
                   const std::string& params,
                   int timeOut)
{
    // Create structures
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );
    // Add .exe suffix
    std::wstring exeFilenameW(exeFilename.begin(), exeFilename.end());
    std::string exeSuffix(".exe");
    exeFilenameW += std::wstring(exeSuffix.begin(), exeSuffix.end());
    std::wstring paramsW(params.begin(), params.end());
    // Attempt to execute
    if( CreateProcessW( exeFilenameW.c_str(),
                        &paramsW[0],
                        nullptr,    // Don't inherit process handle
                        nullptr,    // Don't inherit thread handle
                        0,          // Handle inheritance off
                        0,          // Flags
                        nullptr,    // Parent environment block
                        nullptr,    // Parent starting directory
                        &si,
                        &pi ))
    {
        // Wait until process is finished - but no longer than 10x timeOut value
        int timeOutMs = 1000 * timeOut;
        uint32_t eventType = WaitForSingleObject(pi.hProcess, 10 * timeOutMs);
        if(eventType == WAIT_OBJECT_0)
        {
            DWORD exitCode = 0xFFFFFFFF;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            return int(exitCode);
        }
        else
        {
            throw std::runtime_error("Waited too long on process.");
        }
    }
    else
    {
        // Process invocation failed
        throw std::runtime_error("Failed to invoke process");
    }
}
#endif
