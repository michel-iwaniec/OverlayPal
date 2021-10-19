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
#ifndef SPRITE_H
#define SPRITE_H

#include <set>

#include "Array2D.h"

struct Sprite
{
    int x;
    int y;
    int p;
    std::set<uint8_t> colors;
    Image2D pixels;
    int numBlankPixelsLeft;
    int numBlankPixelsRight;
};

Sprite extractSprite(Image2D& image,
                     size_t xPos,
                     size_t yPos,
                     size_t width,
                     size_t height,
                     const std::set<uint8_t>& colors,
                     uint8_t backgroundColor,
                     bool removePixels);

#endif // SPRITE_H
