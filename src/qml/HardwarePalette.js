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

.pragma library

function palToRGB(palIndex, hardwarePalette)
{
    var rgb = hardwarePalette[palIndex & 0x3F];
    return Qt.rgba(rgb[0]/255.0, rgb[1]/255.0, rgb[2]/255.0, 1.0);
}

function textPalToRGB(palIndexText, hardwarePalette)
{
    var palIndex = parseInt(palIndexText, 16);
    return palToRGB(palIndex, hardwarePalette);
}

function luminance(r, g, b)
{
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

function fgTextPalToRGB(palIndexText, hardwarePalette)
{
    var color = textPalToRGB(palIndexText, hardwarePalette);
    if(luminance(color.r, color.g, color.b) < 0.5)
    {
        // Pick white foreground color
        return Qt.rgba(1.0, 1.0, 1.0, 1.0);
    }
    else
    {
        // Pick black foreground color
        return Qt.rgba(0.0, 0.0, 0.0, 1.0);
    }
}
