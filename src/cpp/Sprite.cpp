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
