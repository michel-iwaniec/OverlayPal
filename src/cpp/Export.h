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
#ifndef EXPORT_H
#define EXPORT_H

#include <vector>
#include <cstdint>

#include "OverlayOptimiser.h"

struct ExportDataNES
{
    std::vector<uint8_t> nametable;
    std::vector<uint8_t> exram;
    std::vector<uint8_t> bgCHR;
    std::vector<uint8_t> oamCHR;
    std::vector<uint8_t> oam;
    std::vector<uint8_t> palette;
};

ExportDataNES buildExportData(const OverlayOptimiser& optimiser, int paletteMask);

#endif // EXPORT_H
