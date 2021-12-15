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

#pragma once
#ifndef SUB_PROCESS_H
#define SUB_PROCESS_H

#include <string>

int executeProcess(std::string exeFilename, std::vector<std::string> params, int timeOut, std::string startingDirectory);

std::string quoteStringOnWindows(const std::string& s);

#endif // SUB_PROCESS_H
