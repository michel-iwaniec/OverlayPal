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
#include "Sprite.h"

//---------------------------------------------------------------------------------------------------------------------

Sprite extractSprite(Image2D& image,
                     size_t xPos,
                     size_t yPos,
                     size_t width,
                     size_t height,
                     const std::set<uint8_t>& colors,
                     uint8_t backgroundColor,
                     bool removePixels)
{
    Sprite s;
    s.pixels = Image2D(width, height);
    s.x = xPos;
    s.y = yPos;
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            uint8_t c = image(xPos + x, yPos + y);
            bool insideImage = xPos + x < image.width() && yPos + y < image.height();
            if(colors.count(c) > 0 && insideImage)
            {
                s.pixels(x, y) = c;
                s.colors.insert(c);
                if(removePixels)
                {
                    image(xPos + x, yPos + y) = backgroundColor;
                }
            }
            else
            {
                s.pixels(x, y) = backgroundColor;
            }
        }
    }
    return s;
}
