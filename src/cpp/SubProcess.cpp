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

#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#endif

#include "SubProcess.h"

#ifdef _WIN32

std::string quoteStringOnWindows(const std::string& s)
{
    return std::string("\"") + s + std::string("\"");
}

std::wstring mergedParams(std::vector<std::string>& params)
{
    size_t length = params.size();
    std::wstring s;
    for(size_t i = 0; i < length; i++)
    {
        s += std::wstring(L" ");
        std::wstring tempS(params[i].begin(), params[i].end());
        s += tempS;
    }
    return s;
}

int executeProcess(std::string exeFilename,
                   std::vector<std::string> params,
                   int timeOut,
                   std::string startingDirectory)
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
    std::wstring paramsW = mergedParams(params);
    // Attempt to execute
    std::wstring startingDirectoryW(startingDirectory.begin(), startingDirectory.end());

    if( CreateProcessW( exeFilenameW.c_str(),
                        &paramsW[0],
                        nullptr,                    // Don't inherit process handle
                        nullptr,                    // Don't inherit thread handle
                        0,                          // Handle inheritance off
                        0,                          // Flags
                        nullptr,                    // Parent environment block
                        startingDirectoryW.c_str(), // Parent starting directory
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
#else

std::string quoteStringOnWindows(const std::string& s)
{
    return s;
}

std::vector<char*> splitParams(std::vector<std::string>& params)
{
    size_t length = params.size();
    std::vector<char*> charPtrs;
    for(size_t i = 0; i < length; i++)
    {
        params.push_back(params[i].c_str());
    }
    charPtrs.push_back(nullptr);
    return charPtrs;
}

int executeProcess(std::string exeFilename,
                   std::vector<std::string> params,
                   int timeOut,
                   std::string startingDirectory)
{
    // Split space-separated parameters into individual asciiz strings to create argv
    std::vector<char*> ptrs = splitParams(params);
    char* exeFilenameP = exeFilename.data();
    ptrs.insert(ptrs.begin(), 1, exeFilenameP);
    char** argv = ptrs.data();
    // Fork new process for execv call
    pid_t pid = fork();
    if(pid == -1)
    {
        throw std::runtime_error("fork() failed in executeProcess");
    }
    else if(pid == 0)
    {
        // Run program in forked process - new process also terminates here
        execv(exeFilename.c_str(), argv);
    }
    else
    {
        // Wait for forked process to finish
        int exitCode = -1;
        waitpid(pid, &exitCode, 0);
        return exitCode;
    }
}
#endif
