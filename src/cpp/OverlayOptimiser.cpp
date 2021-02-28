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
#include <iostream>
#include <iomanip>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <cstdio>
#include <array>

#include "SubProcess.h"

#include "OverlayOptimiser.h"

//---------------------------------------------------------------------------------------------------------------------

OverlayOptimiser::OverlayOptimiser():
    mBackgroundColor(0)
{

}

//---------------------------------------------------------------------------------------------------------------------

void OverlayOptimiser::setExecutablePath(const std::string& executablePath)
{
    mExecutablePath = executablePath;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayOptimiser::setWorkPath(const std::string& workPath)
{
    mWorkPath = workPath;
}

//---------------------------------------------------------------------------------------------------------------------

std::string OverlayOptimiser::exePathFilename(const std::string& exeFilename) const
{
    return mExecutablePath + "/" + exeFilename;
}

//---------------------------------------------------------------------------------------------------------------------

std::string OverlayOptimiser::workPathFilename(const std::string& workFilename) const
{
    return mWorkPath + "/" + workFilename;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayOptimiser::writeCmplLayerData(std::ofstream& f, const std::string& name, const GridLayer& layer, std::function<int(int, int, int)> const& callback)
{
    f << "%" << name.c_str() << "[XRANGE, YRANGE, COLORS] <\n";
    for(int x = 0; x < layer.width(); x++)
    {
        for(int y = 0; y < layer.height(); y++)
        {
            for(auto c : layer.colors())
            {
                int v = callback(x, y, c);
                f << v << " ";
            }
            f << "\n";
        }
    }
    f << ">\n";
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayOptimiser::writeCmplDataFile(const GridLayer& layer, int gridCellColorLimit, int maxBackgroundPalettes, int maxSpritePalettes, int maxRowSize, const std::string& filename)
{
    std::ofstream f(filename, std::ofstream::out);
    if(!f)
    {
        throw std::runtime_error(std::string("Failed to open file '") + filename + "' for writing CMPL input data.");
    }
    // Limits
    f << "%CELL_COLOR_LIMIT < " << gridCellColorLimit << " >\n";
    f << "%MAX_BG_PALETTES < " << maxBackgroundPalettes << " >\n";
    f << "%MAX_SPR_PALETTES < " << maxSpritePalettes << " >\n";
    f << "%OVERLAY_ROW_SIZE_LIMIT < " << maxRowSize << " >\n";
    // X / Y ranges
    f << "%XRANGE set < 0.." << layer.width()-1 << " >\n";
    f << "%YRANGE set < 0.." << layer.height()-1 << " >\n";
    // All colors present in layer
    f << "%COLORS set < ";
    for(auto c : layer.colors())
    {
        f << int(c) << " ";
    }
    f << " >\n";
    // layerColors
    writeCmplLayerData(f, "layerColors", layer, [&](int x, int y, uint8_t c) { return layer(x, y).colors.count(c) ? 1 : 0; });
    writeCmplLayerData(f, "layerColorColumnCount", layer, [&](int x, int y, uint8_t c) { return layer(x, y).colors.count(c) ? layer(x, y).columnCount.at(c) : 0; });
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayOptimiser::runCmplProgram(const std::string& inputFilename,
                                      const std::string& outputFilename,
                                      const std::string& solutionCsvFilename,
                                      int timeOut)
{
    // Make a copy of the original program and prepend timeOut parameter to it
    // This is an ugly work-around for there being no other way(?) to set the CBC timeout parameter :(
    std::ifstream inputFile(inputFilename, std::ifstream::in);
    std::string inputFileStr((std::istreambuf_iterator<char>(inputFile)),
                              std::istreambuf_iterator<char>());
    inputFile.close();
    std::ofstream outputFile(outputFilename);
    outputFile << "%opt cbc seconds " << timeOut << "\n";
    outputFile << inputFileStr;
    outputFile.close();
    // Execute process
    std::string cmplExecutable("Cmpl/bin/cmpl");
    std::string params;
    params += " -i " + outputFilename;
    params += " -solutionCsv " + solutionCsvFilename;
    int exitCode = executeProcess(exePathFilename(cmplExecutable), params, timeOut);
    if(exitCode != 0)
    {
        throw Error("Non-zero exit code from CMPL");
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayOptimiser::parseSolutionValue(const std::string& line, std::vector<int>& indices, int& value)
{
    indices.clear();
    size_t arrayStartPos = line.find("[", 0);
    size_t arrayEndPos = line.find("]", 0);
    size_t activityPos = line.find(";B;", 0);
    if(arrayStartPos != std::string::npos &&
       arrayEndPos != std::string::npos &&
       activityPos != std::string::npos)
    {
        assert(arrayStartPos+1 < line.size());
        assert(arrayEndPos+1 < line.size());
        assert(activityPos+3 < line.size());
        assert(arrayStartPos < arrayEndPos);
        // Read indices
        {
            std::string indicesStr = line.substr(arrayStartPos + 1, arrayEndPos - arrayStartPos + 1); //arrayStartPos
            char c;
            int i;
            std::stringstream ss(indicesStr);
            while(true)
            {
                ss >> i >> c;
                indices.push_back(i);
                if(c != ',')
                    break;
            }
        }
        // Read activity
        {
            std::string activityStr = line.substr(activityPos+3, line.size() - (activityPos + 3));
            std::stringstream ss(activityStr);
            ss >> value;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayOptimiser::parseCmplSolution(const std::string& csvFilename,
                                         std::vector<std::set<uint8_t>>& palettes,
                                         GridLayer& colorsBackground,
                                         GridLayer& colorsOverlay,
                                         Array2D<uint8_t>& paletteIndicesBackground,
                                         bool secondPass)
{
    std::ifstream f;
    f.open(csvFilename, std::ifstream::in);
    if(!f)
    {
        throw std::runtime_error(std::string("Failed to open solution file: ") + csvFilename);
    }
    // Read data
    const std::string colorsBackgroundPrefix = secondPass ? "colorsOverlayGrid[" : "colorsBG[";
    const std::string colorsOverlayPrefix = secondPass ? "colorsOverlayFree[" : "colorsOverlay[";
    const std::string palettesNamePrefix = secondPass ? "palettesOverlay[" : "palettesBG[";
    const std::string usesPalettePrefix = secondPass ? "usesPaletteOverlay[" : "usesPaletteBG[";
    std::string line;
    std::getline(f, line);
    if(line.find("CMPL csv export") == std::string::npos)
    {
        throw std::runtime_error(std::string("Solution file header unrecognized"));
    }
    // Parse values
    palettes.clear();
    std::vector<int> indices;
    int value;
    while(true)
    {
        if(!std::getline(f, line))
            break;
        if(line.rfind(colorsBackgroundPrefix, 0) == 0)
        {
            parseSolutionValue(line, indices, value);
            assert(indices.size() == 3 && "colors background index length mismatch");
            if(value == 1)
            {
                int x = indices[0];
                int y = indices[1];
                int c = indices[2];
                colorsBackground(x, y).colors.insert(c);
            }
        }
        else if(line.rfind(colorsOverlayPrefix, 0) == 0)
        {
            parseSolutionValue(line, indices, value);
            assert(indices.size() == 3 && "colors overlay index length mismatch");
            if(value == 1)
            {
                int x = indices[0];
                int y = indices[1];
                int c = indices[2];
                colorsOverlay(x, y).colors.insert(c);
            }
        }
        else if(line.rfind(palettesNamePrefix, 0) == 0)
        {
            parseSolutionValue(line, indices, value);
            assert(indices.size() == 2 && "Palette index length mismatch");
            if(value == 1)
            {
                int p = indices[0];
                int c = indices[1];
                if(p >= palettes.size())
                {
                    palettes.resize(p + 1);
                }
                palettes[p].insert(c);
            }
        }
        else if(line.rfind(usesPalettePrefix, 0) == 0)
        {
            parseSolutionValue(line, indices, value);
            assert(indices.size() == 3 && "Uses-palette index length mismatch");
            if(value == 1)
            {
                int x = indices[0];
                int y = indices[1];
                int paletteIndex = indices[2];
                paletteIndicesBackground(x, y) = paletteIndex + (secondPass ? 4 : 0);
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayOptimiser::zeroEmptyPaletteIndices(Array2D<uint8_t>& paletteIndices, const GridLayer& layer)
{
    assert(paletteIndices.width() == layer.width() && paletteIndices.height() == layer.height());
    for(size_t y = 0; y < layer.height(); y++)
    {
        for(size_t x = 0; x < layer.width(); x++)
        {
            if(layer(x, y).colors.size() == 0)
            {
                paletteIndices(x, y) = 0;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

const Array2D<uint8_t> &OverlayOptimiser::debugPaletteIndicesBackground() const
{
    return mPaletteIndicesBackground;
}

//---------------------------------------------------------------------------------------------------------------------

const GridLayer &OverlayOptimiser::layerBackground() const
{
    return mLayerBackground;
}

//---------------------------------------------------------------------------------------------------------------------

const GridLayer &OverlayOptimiser::layerOverlay() const
{
    return mLayerOverlay;
}

//---------------------------------------------------------------------------------------------------------------------

void moveOverlayColors(const Image2D& inputImage,
                  Image2D& imageBackground,
                  Image2D& imageOverlay,
                  GridLayer& layerOverlay,
                  uint8_t backgroundColor)
{
    int cellWidth = layerOverlay.cellWidth();
    int cellHeight = layerOverlay.cellHeight();
    for(size_t y = 0; y < layerOverlay.height(); y++)
    {
        for(size_t x = 0; x < layerOverlay.width(); x++)
        {
            for(size_t i = 0; i < layerOverlay.cellHeight(); i++)
            {
                for(size_t j = 0; j < layerOverlay.cellWidth(); j++)
                {
                    int xx = x * cellWidth + j;
                    int yy = y * cellHeight + i;
                    uint8_t c = inputImage(xx, yy);
                    imageOverlay(xx, yy) = backgroundColor;
                    imageBackground(xx, yy) = inputImage(xx, yy);
                    if(layerOverlay(x, y).colors.count(c) > 0)
                    {
                        imageOverlay(xx, yy) = c;
                        imageBackground(xx, yy) = backgroundColor;
                    }
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayOptimiser::consistentLayers(const Image2D& image, const GridLayer& layer, const std::vector<std::set<uint8_t>>& palettes, const Array2D<uint8_t>& paletteIndices, uint8_t backgroundColor)
{
    assert(layer.width() == paletteIndices.width() && layer.height() == paletteIndices.height());
    const size_t w = layer.width();
    const size_t h = layer.height();
    for(size_t y = 0; y < h; y++)
    {
        for(size_t x = 0; x < w; x++)
        {
            uint8_t paletteIndex = paletteIndices(x, y);
            assert(paletteIndex < palettes.size());
            for(uint8_t c : layer(x, y).colors)
            {
                bool colorInPalette = palettes[paletteIndex].count(c) > 0;
                assert(colorInPalette && "GridCell colors must be present in palette");
                if(!colorInPalette)
                    return false;
            }
            //
            for(size_t i = 0; i < layer.cellHeight(); i++)
            {
                for(size_t j = 0; j < layer.cellWidth(); j++)
                {
                    size_t xx = layer.cellWidth() * x + j;
                    size_t yy = layer.cellHeight() * y + i;
                    uint8_t c = image(xx, yy);
                    if(c != backgroundColor)
                    {
                        bool colorInGridCell = layer(x, y).colors.count(c) > 0;
                        assert(colorInGridCell && "Pixel colors must be present in grid cell");
                        if(!colorInGridCell)
                            return false;
                    }
                }
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayOptimiser::fillMissingPaletteGroups(std::vector<std::set<uint8_t>>& palettes)
{
    while(palettes.size() < 4)
    {
        palettes.push_back(std::set<uint8_t>());
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayOptimiser::convertFirstPass(const Image2D& image,
                                        int gridCellColorLimit,
                                        int maxBackgroundPalettes,
                                        int maxSpritePalettes,
                                        int maxRowSize,
                                        int timeOut,
                                        GridLayer& layer,
                                        GridLayer& layerBackground,
                                        GridLayer& layerOverlay,
                                        std::vector<std::set<uint8_t>>& palettesBG,
                                        Array2D<uint8_t>& paletteIndicesBackground)
{
    // Make layer for input image
    writeCmplDataFile(layer,
                      gridCellColorLimit,
                      maxBackgroundPalettes,
                      maxSpritePalettes,
                      maxRowSize,
                      workPathFilename(firstPassDataFilename));
    //
    runCmplProgram(exePathFilename(firstPassProgramInputFilename),
                   workPathFilename(firstPassProgramOutputFilename),
                   workPathFilename(firstPassSolutionFilename),
                   timeOut);
    if(!parseCmplSolution(workPathFilename(firstPassSolutionFilename),
                          palettesBG,
                          layerBackground,
                          layerOverlay,
                          paletteIndicesBackground,
                          false))
    {
        throw Error("Failed to parse CMPL result (first pass)");
    }
    fillMissingPaletteGroups(palettesBG);
    zeroEmptyPaletteIndices(paletteIndicesBackground, layerBackground);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayOptimiser::convertSecondPass(int gridCellColorLimit,
                                         int maxSpritePalettes,
                                         int maxSpritesPerScanline,
                                         int timeOut,
                                         GridLayer& layer,
                                         GridLayer& layerOverlayGrid,
                                         GridLayer& layerOverlayFree,
                                         std::vector<std::set<uint8_t>>& palettes,
                                         Array2D<uint8_t>& paletteIndicesOverlay)
{
    writeCmplDataFile(layer,
                      gridCellColorLimit,
                      0,
                      maxSpritePalettes,
                      2 * maxSpritesPerScanline,
                      workPathFilename(secondPassDataFilename));
    //
    runCmplProgram(exePathFilename(secondPassProgramInputFilename),
                   workPathFilename(secondPassProgramOutputFilename),
                   workPathFilename(secondPassSolutionFilename),
                   timeOut);
    std::vector<std::set<uint8_t>> palettesSPR;
    if(!parseCmplSolution(workPathFilename(secondPassSolutionFilename),
                          palettesSPR,
                          layerOverlayGrid,
                          layerOverlayFree,
                          paletteIndicesOverlay,
                          true))
    {
        throw Error("Failed to parse CMPL result (second pass)");
    }
    fillMissingPaletteGroups(palettesSPR);
    zeroEmptyPaletteIndices(paletteIndicesOverlay, layerOverlayGrid);
    for(const std::set<uint8_t>& palette : palettesSPR)
    {
        palettes.push_back(palette);
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayOptimiser::convert(const Image2D& image,
                               uint8_t backgroundColor,
                               int gridCellColorLimit,
                               int maxBackgroundPalettes,
                               int maxSpritePalettes,
                               int maxSpritesPerScanline,
                               int timeOut)
{
    // Remove old files
    std::array<const char*, 6> filenames = {
        firstPassProgramOutputFilename,
        firstPassDataFilename,
        firstPassSolutionFilename,
        secondPassProgramOutputFilename,
        secondPassDataFilename,
        secondPassSolutionFilename
    };
    for(const char* filename : filenames)
    {
        remove(workPathFilename(filename).c_str());
    }
    // +1 to give some extra leeway due to using a 16x16 grid in first pass
    int maxRowSize = (maxSpritesPerScanline * SpriteWidth / GridCellWidth) + 1;
    // Execute first pass
    GridLayer layer(backgroundColor, GridCellWidth, GridCellHeight, image);
    GridLayer layerBackground(backgroundColor, layer.cellWidth(), layer.cellHeight(), layer.width(), layer.height());
    GridLayer layerOverlay(backgroundColor, layer.cellWidth(), layer.cellHeight(), layer.width(), layer.height());
    std::vector<std::set<uint8_t>> palettes;
    Array2D<uint8_t> paletteIndicesBackground(layer.width(), layer.height());
    bool successPassOne = convertFirstPass(image,
                                           gridCellColorLimit,
                                           maxBackgroundPalettes,
                                           maxSpritePalettes,
                                           maxRowSize,
                                           timeOut,
                                           layer,
                                           layerBackground,
                                           layerOverlay,
                                           palettes,
                                           paletteIndicesBackground);
    if(!successPassOne)
        throw Error("First pass failed.");
    // Split image
    Image2D imageBackground(image.width(), image.height());
    Image2D imageOverlay(image.width(), image.height());
    moveOverlayColors(image, imageBackground, imageOverlay, layerOverlay, backgroundColor);

    assert(consistentLayers(imageBackground, layerBackground, palettes, paletteIndicesBackground, backgroundColor));
    assert(!image.empty(mBackgroundColor));
    assert(!imageBackground.empty(mBackgroundColor));
    assert(!imageOverlay.empty(mBackgroundColor));

    // Re-initialise for cached data - and halve GridCellWidth
    int OverlayGridCellWidth = GridCellWidth / 2;
    int OverlayWidth = layer.width() * 2;
    layerOverlay = GridLayer(backgroundColor, OverlayGridCellWidth, GridCellHeight, imageOverlay);
    Array2D<uint8_t> paletteIndicesOverlay(OverlayWidth, layerOverlay.height());
    // Second pass
    Image2D imageOverlayGrid(image.width(), image.height());
    Image2D imageOverlayFree(image.width(), image.height());
    GridLayer layerOverlayGrid(backgroundColor, OverlayGridCellWidth, layer.cellHeight(), OverlayWidth, layer.height());
    GridLayer layerOverlayFree(backgroundColor, OverlayGridCellWidth, layer.cellHeight(), OverlayWidth, layer.height());
    bool successPassTwo = convertSecondPass(gridCellColorLimit,
                                            maxSpritePalettes,
                                            maxSpritesPerScanline,
                                            timeOut,
                                            layerOverlay,
                                            layerOverlayGrid,
                                            layerOverlayFree,
                                            palettes,
                                            paletteIndicesOverlay);
    if(!successPassTwo)
        throw Error("Second pass failed.");
    moveOverlayColors(imageOverlay, imageOverlayGrid, imageOverlayFree, layerOverlayFree, backgroundColor);
    assert(consistentLayers(imageOverlayGrid, layerOverlayGrid, palettes, paletteIndicesOverlay, backgroundColor));
    // Copy state to persistent members
    mLayerBackground = layerBackground;
    mLayerOverlay = layerOverlayGrid;
    mLayerOverlayFree = layerOverlayFree;
    mPaletteIndicesBackground = paletteIndicesBackground;
    mPaletteIndicesOverlay = paletteIndicesOverlay;
    mBackgroundColor = backgroundColor;
    assert(!image.empty(mBackgroundColor));
    assert(!imageBackground.empty(mBackgroundColor));
    assert(!imageOverlay.empty(mBackgroundColor));
    mOutputImage = image;
    mOutputImageBackground = imageBackground;
    mOutputImageOverlay = imageOverlayGrid;
    mOutputImageOverlayFree = imageOverlayFree;
    assert(!mOutputImage.empty(mBackgroundColor));
    assert(!mOutputImageBackground.empty(mBackgroundColor));
    assert(!mOutputImageOverlay.empty(mBackgroundColor));
    mPalettes = palettes;
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayOptimiser::conversionSuccessful() const
{
    return mConversionSuccessful;
}

//---------------------------------------------------------------------------------------------------------------------

Image2D OverlayOptimiser::outputImageBackground() const
{
    assert(!mOutputImage.empty(mBackgroundColor));
    assert(!mOutputImageBackground.empty(mBackgroundColor));
    assert(!mOutputImageOverlay.empty(mBackgroundColor));
    return remapColors(mOutputImageBackground, mLayerBackground, mPalettes, mPaletteIndicesBackground);
}

//---------------------------------------------------------------------------------------------------------------------

Image2D OverlayOptimiser::outputImageOverlay() const
{
    assert(!mOutputImage.empty(mBackgroundColor));
    assert(!mOutputImageBackground.empty(mBackgroundColor));
    assert(!mOutputImageOverlay.empty(mBackgroundColor));
    return remapColors(mOutputImageOverlay, mLayerOverlay, mPalettes, mPaletteIndicesOverlay);
}

//---------------------------------------------------------------------------------------------------------------------

Image2D OverlayOptimiser::outputImage() const
{
    assert(!mOutputImage.empty(mBackgroundColor));
    assert(!mOutputImageBackground.empty(mBackgroundColor));
    assert(!mOutputImageOverlay.empty(mBackgroundColor));
    Image2D background = outputImageBackground();
    Image2D overlay = outputImageOverlay();
    assert(!background.empty());
    assert(!overlay.empty());
    assert(background.width() == overlay.width() && background.height() == overlay.height());
    size_t w = background.width();
    size_t h = background.height();
    Image2D image(w, h);
    for(size_t y = 0; y < h; y++)
    {
        for(size_t x = 0; x < w; x++)
        {
            uint8_t cB = background(x, y);
            uint8_t cO = overlay(x, y);
            assert(!((cB != 0) && (cO != 0)));
            if(cO != 0)
            {
                image(x, y) = cO;
            }
            else if(cB != 0)
            {
                image(x, y) = cB;
            }
            else
            {
                image(x, y) = 0;
            }
        }
    }
    return image;
}

//---------------------------------------------------------------------------------------------------------------------

Image2D OverlayOptimiser::remapColors(const Image2D& image,
                                      const GridLayer& layer,
                                      const std::vector<std::set<uint8_t>>& palettes,
                                      const Array2D<uint8_t>& paletteIndices) const
{
    assert(!image.empty(mBackgroundColor));
    // Create per-cell mapping
    size_t gridWidth = paletteIndices.width();
    size_t gridHeight = paletteIndices.height();
    Array2D<std::unordered_map<uint8_t, uint8_t>> perCellMappingForward(gridWidth, gridHeight);
    for(size_t y = 0; y < gridHeight; y++)
    {
        for(size_t x = 0; x < gridWidth; x++)
        {
            uint8_t paletteIndex = paletteIndices(x, y);
            size_t j = 1;
            for(uint8_t c : palettes[paletteIndex])
            {
                perCellMappingForward(x, y)[c] = paletteIndex * PaletteGroupSize + j;
                j++;
            }
        }
    }
    // Map each pixel
    Image2D rImage(image.width(), image.height());
    for(size_t y = 0; y < image.height(); y++)
    {
        for(size_t x = 0; x < image.width(); x++)
        {
            size_t cellX = x / layer.cellWidth();
            size_t cellY = y / layer.cellHeight();
            const std::unordered_map<uint8_t, uint8_t>& m = perCellMappingForward(cellX, cellY);
            uint8_t c = image(x, y);
            if(m.find(c) != m.end())
            {
                c = m.at(c);
                rImage(x, y) = c;
            }
            else
            {
                assert(c == mBackgroundColor);
                rImage(x, y) = 0;
            }
        }
    }
    assert(!rImage.empty());
    return rImage;
}

//---------------------------------------------------------------------------------------------------------------------

const std::vector<std::set<uint8_t>> &OverlayOptimiser::palettes() const
{
    return mPalettes;
}

//---------------------------------------------------------------------------------------------------------------------

std::vector<OverlayOptimiser::Sprite> OverlayOptimiser::spritesOverlay() const
{
    const GridLayer& layer = mLayerOverlay;
    const Array2D<uint8_t>& paletteIndicesOverlay = mPaletteIndicesOverlay;
    std::vector<Sprite> sprites;
    for(size_t y = 0; y < layer.height(); y++)
    {
        for(size_t x = 0; x < layer.width(); x++)
        {
            if(layer(x, y).colors.size() > 0)
            {
                OverlayOptimiser::Sprite s;
                s.x = x * layer.cellWidth();
                s.y = y * layer.cellHeight();
                s.p = paletteIndicesOverlay(x, y);
                s.colors = layer(x, y).colors;
                sprites.push_back(s);
            }
        }
    }
    return sprites;
}
