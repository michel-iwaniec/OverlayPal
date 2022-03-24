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

#include <QtConcurrent/QtConcurrentRun>
#include <QQmlEngine>
#include <QUrl>
#include <QDebug>
#include <QStandardPaths>

#include <iostream>
#include <iomanip>
#include <limits>

#include "OverlayPalApp.h"
#include "Export.h"
#include "GridLayer.h"
#include "ImageUtils.h"
#include "OverlayOptimiser.h"

#include "OverlayPalGuiBackend.h"

//---------------------------------------------------------------------------------------------------------------------

Image2D OverlayPalGuiBackend::qImageToImage2D(const QImage& qImage)
{
    const int w = qImage.width();
    const int h = qImage.height();
    Image2D image(w, h);
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            uint8_t c = qImage.pixelIndex(x, y);
            image(x, y) = c;
        }
    }
    return image;
}

//---------------------------------------------------------------------------------------------------------------------

QImage OverlayPalGuiBackend::image2DToQImage(const Image2D& image, const QVector<QRgb>& colorTable)
{
    const int w = image.width();
    const int h = image.height();
    QImage qImage(w, h, QImage::Format_Indexed8);
    qImage.setColorTable(colorTable);
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            uint8_t c = image(x, y);
            assert(c < colorTable.size());
            qImage.setPixel(x, y, c);
        }
    }
    return qImage;
}

//---------------------------------------------------------------------------------------------------------------------

OverlayPalGuiBackend::OverlayPalGuiBackend(QObject *parent):
    QObject(parent),
    mUniqueColors(false),
    mTimeOut(60),
    mTrackInputImage(false),
    mShiftX(0),
    mShiftY(0),
    mPreventBlackerThanBlack(true),
    mMapInputColors(true),
    mConversionInProgress(false),
    mProcessingInputImage(false),
    mHardwarePaletteName("palgen"),
    mOutputImage(ScreenWidth, ScreenHeight, QImage::Format_Indexed8),
    mBackgroundColor(0),
    mInputImage(ScreenWidth, ScreenHeight, QImage::Format_Indexed8),
    mInputImageIndexed(ScreenWidth, ScreenHeight, QImage::Format_Indexed8),
    mGridCellWidth(16),
    mGridCellHeight(16)
{
    QVector<QRgb> dummyColorTable;
    dummyColorTable.push_back(0);
    mInputImage.setColorTable(dummyColorTable);
    mInputImage.fill(0);
    mInputImageIndexed.setColorTable(dummyColorTable);
    mInputImageIndexed.fill(0);
    mOutputImage.setColorTable(dummyColorTable);
    mOutputImage.fill(0);
    QObject::connect(&mInputFileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(handleInputFileChanged(QString)));
    std::string executablePath = QCoreApplication::applicationDirPath().toStdString();
    mOverlayOptimiser.setExecutablePath(executablePath);

#ifndef _WIN32
    // On MacOS and GNU/Linux, use a separate Qt-provided directory for temporary data files
    QString pathToTmp = qApp->appStoragePath();
    mOverlayOptimiser.setWorkPath(pathToTmp.toStdString());
#else
    // On Windows, put temporary data files in Cmpl/bin.
    // This is to sidestep a weird CMPL2-bug, which prevents CMPL2 from
    // locating files when CMPL2 is running as a subprocess.
    // TODO: Investigate root cause of this bug.
    mOverlayOptimiser.setWorkPath(executablePath + "/" + "Cmpl/bin");
#endif
    loadHardwarePalettes(QString(executablePath.c_str()) + QString("/nespalettes"));
    // Prevent QML engine from taking ownership of and destroying models
    QQmlEngine::setObjectOwnership(&mPaletteModel, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(&mHardwarePaletteNamesModel, QQmlEngine::CppOwnership);
    QQmlEngine::setObjectOwnership(&mInputImageHardwareColorsModel, QQmlEngine::CppOwnership);
}

//---------------------------------------------------------------------------------------------------------------------

OverlayPalGuiBackend::~OverlayPalGuiBackend()
{
}

//---------------------------------------------------------------------------------------------------------------------

const QString& OverlayPalGuiBackend::hardwarePaletteName() const
{
    return mHardwarePaletteName;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setHardwarePaletteName(const QString& hardwarePaletteName)
{
    assert(mHardwarePalettes.contains(hardwarePaletteName));
    if(hardwarePaletteName != mHardwarePaletteName)
    {
        mHardwarePaletteName = hardwarePaletteName;
        quantizeInputImage();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::loadHardwarePalette(const QFileInfo& fileInfo)
{
    QFile palFile(fileInfo.absoluteFilePath());
    if(!palFile.open(QIODevice::ReadOnly))
        return;
    QByteArray palData = palFile.readAll();
    QVariantList hardwarePalette;
    for(int i = 0; i < HardwarePaletteSize; i++)
    {
        uint8_t r = palData[3 * i + 0];
        uint8_t g = palData[3 * i + 1];
        uint8_t b = palData[3 * i + 2];
        QVariantList rgbList;
        rgbList.push_back(int(r));
        rgbList.push_back(int(g));
        rgbList.push_back(int(b));
        hardwarePalette.push_back(rgbList);
    }
    mHardwarePalettes[fileInfo.baseName()] = hardwarePalette;
    mHardwarePaletteNames.append(fileInfo.baseName());
    palFile.close();
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::loadHardwarePalettes(const QString& palettesPath)
{
    QDir palDir(palettesPath);
    QFileInfoList palFileInfos = palDir.entryInfoList();
    for(auto& palFileInfo : palFileInfos)
    {
        if(palFileInfo.suffix() == "pal" && palFileInfo.size() == 3 * HardwarePaletteSize)
        {
            loadHardwarePalette(palFileInfo);
        }
    }
    mHardwarePaletteNamesModel.setStringList(mHardwarePaletteNames);
    // Use first loaded palette
    mHardwarePaletteName = mHardwarePalettes.firstKey();
    mInputImageHardwareColorsModel.setHardwarePalette(mHardwarePalettes[mHardwarePaletteName]);
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::handleInputFileChanged(const QString& filename)
{
    if(mTrackInputImage)
    {
        if(!mProcessingInputImage)
        {
            mProcessingInputImage = true;
            mInputImageFilename = filename;
            if(mInputImage.load(mInputImageFilename))
            {
                assert(mInputImage.width() > 0 && mInputImage.height() > 0);
                quantizeInputImage();
            }
            mProcessingInputImage = false;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

QImage OverlayPalGuiBackend::cropOrExtendImage(const QImage& image, uint8_t backgroundColor)
{
    QImage copy(ScreenWidth, ScreenHeight, QImage::Format_Indexed8);
    copy.setColorTable(image.colorTable());
    size_t colorTableSize = image.colorTable().size();
    assert(backgroundColor < colorTableSize);
    for(int y = 0; y < ScreenHeight; y++)
    {
        for(int x = 0; x < ScreenWidth; x++)
        {
            if(x < image.width() && y < image.height())
            {
                // Use source pixel
                uint8_t pixelIndex = image.pixelIndex(x, y);
                copy.setPixel(x, y, pixelIndex);
            }
            else
            {
                // Out-of-range - use background color
                copy.setPixel(x, y, backgroundColor);
            }
        }
    }
    return copy;
}

//---------------------------------------------------------------------------------------------------------------------

uint8_t OverlayPalGuiBackend::detectBackgroundColor(const QImage& image, uint8_t oldBackgroundColor)
{
    bool oldBackgroundColorInImage = false;
    for(int y = 0; y < image.height(); y++)
    {
        for(int x = 0; x < image.width(); x++)
        {
            if(image.pixelIndex(x, y) == oldBackgroundColor)
            {
                oldBackgroundColorInImage = true;
                break;
            }
        }
    }
    if(!oldBackgroundColorInImage)
    {
        assert(image.width() > 0 && image.height() > 0);
        uint8_t newBackgroundColor = image.pixelIndex(0, 0);
        assert(newBackgroundColor < image.colorTable().size());
        assert(newBackgroundColor < HardwarePaletteSize);
        return newBackgroundColor;
    }
    else
    {
        assert(oldBackgroundColor < HardwarePaletteSize);
        return oldBackgroundColor;
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::quantizeInputImage()
{
    if(potentialHardwarePaletteIndexedImage() && !mMapInputColors)
    {
        // Input is hardware palette values - just use as-s
        mInputImageIndexed = mInputImage;
        mInputImageIndexed.setColorTable(makeColorTableFromHardwarePalette());
    }
    else
    {
        // If attempt to not map failed, this must be an RGB image
        if(!mMapInputColors)
        {
            mMapInputColors = true;
            emit mapInputColorsChanged();
        }
        // Input image is either RGB or unrelated indexed-colors - need to quantize
        // First quantize to 256 colors for simplicity
        QImage inputImageQuantized = mInputImage.convertToFormat(QImage::Format_Indexed8, Qt::ThresholdDither);
        // Then remap to NES palette values
        mInputImageIndexed = remapColorsToNES(inputImageQuantized, mBackgroundColor);
    }
    assert(mInputImageIndexed.width() > 0 && mInputImageIndexed.height() > 0);
    // detect background color from current / available colors
    uint8_t backgroundColor = detectBackgroundColor(mInputImageIndexed, mBackgroundColor);
    //
    mInputImageHardwareColorsModel.setHardwarePalette(mHardwarePalettes[mHardwarePaletteName]);
    // Collect hardware palette values from input image
    std::set<uint8_t> colors;
    for(int y = 0; y < mInputImageIndexed.height(); y++)
    {
        for(int x = 0; x < mInputImageIndexed.width(); x++)
        {
            uint8_t c = mInputImageIndexed.pixelIndex(x, y);
            colors.insert(c);
        }
    }
    mInputImageHardwareColorsModel.setColors(colors);
    // crop image
    mInputImageIndexedBeforeShift = cropOrExtendImage(mInputImageIndexed, backgroundColor);
    // Shift image by current shift values
    mInputImageIndexed = shiftQImage(mInputImageIndexedBeforeShift);
    emit inputImageChanged();
    // Make sure backgroundColorChanged is set and emitted once more after inputImageChanged
    mBackgroundColor = backgroundColor;
    emit backgroundColorChanged();
}

//---------------------------------------------------------------------------------------------------------------------

QString OverlayPalGuiBackend::inputImageFilename() const
{
    return mInputImageFilename;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setInputImageFilename(const QString& inputImageFilenameUrl)
{
    QString inputImageFilename(urlToLocal(inputImageFilenameUrl));
    if(inputImageFilename != mInputImageFilename)
    {
        if(mInputFileWatcher.files().size() > 0)
        {
            mInputFileWatcher.removePath(mInputImageFilename);
        }
        mInputFileWatcher.addPath(inputImageFilename);
    }
    mInputImageFilename = inputImageFilename;
    mInputImage = QImage(mInputImageFilename);
    quantizeInputImage();
}

//---------------------------------------------------------------------------------------------------------------------

QString OverlayPalGuiBackend::validateInputImage(const QString& inputImageFilenameUrl) const
{
    QString inputImageFilename(urlToLocal(inputImageFilenameUrl));
    QImage image(inputImageFilename);
    if(image.isNull())
        return "Invalid image";
    return "";
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayPalGuiBackend::trackInputImage() const
{
    return mTrackInputImage;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setTrackInputImage(bool trackInputImage)
{
    mTrackInputImage = trackInputImage;
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayPalGuiBackend::potentialHardwarePaletteIndexedImage() const
{
    // Image must be indexed
    if(mInputImage.format() != QImage::Format_Indexed8)
        return false;
    // ...and have no color indices above the hardware palette size
    int w = mInputImage.width();
    int h = mInputImage.height();
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            if(mInputImage.pixelIndex(x, y) >= HardwarePaletteSize)
            {
                return false;
            }
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayPalGuiBackend::mapInputColors() const
{
    return mMapInputColors;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setMapInputColors(bool mapInputColors)
{
    if(mapInputColors != mMapInputColors)
    {
        mMapInputColors = mapInputColors;
        quantizeInputImage();
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayPalGuiBackend::uniqueColors() const
{
    return mUniqueColors;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setUniqueColors(bool uniqueColors)
{
    if(uniqueColors != mUniqueColors)
    {
        mUniqueColors = uniqueColors;
        quantizeInputImage();
    }
}

//---------------------------------------------------------------------------------------------------------------------

int OverlayPalGuiBackend::backgroundColor() const
{
    return mBackgroundColor;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setBackgroundColor(int backgroundColor)
{
    mBackgroundColor = static_cast<uint8_t>(backgroundColor);
}

//---------------------------------------------------------------------------------------------------------------------

int OverlayPalGuiBackend::shiftX() const
{
    return mShiftX;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setShiftX(const int &shiftX)
{
    if(shiftX != mShiftX)
    {
        mShiftX = shiftX;
        quantizeInputImage();
    }
}

//---------------------------------------------------------------------------------------------------------------------

int OverlayPalGuiBackend::shiftY() const
{
    return mShiftY;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setShiftY(const int &shiftY)
{
    if(shiftY != mShiftY)
    {
        mShiftY = shiftY;
        quantizeInputImage();
    }
}

//---------------------------------------------------------------------------------------------------------------------

QSize OverlayPalGuiBackend::cellSize() const
{
    return QSize(mGridCellWidth, mGridCellHeight);
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setCellSize(QSize cellSize)
{
    mGridCellWidth = cellSize.width();
    mGridCellHeight = cellSize.height();
}

//---------------------------------------------------------------------------------------------------------------------

int OverlayPalGuiBackend::spriteHeight() const
{
    return mSpriteHeight;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setSpriteHeight(int spriteHeight)
{
    mSpriteHeight = spriteHeight;
}

//---------------------------------------------------------------------------------------------------------------------

int OverlayPalGuiBackend::maxBackgroundPalettes() const
{
    return mMaxBackgroundPalettes;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setMaxBackgroundPalettes(int maxBackgroundPalettes)
{
    mMaxBackgroundPalettes = maxBackgroundPalettes;
}

//---------------------------------------------------------------------------------------------------------------------


int OverlayPalGuiBackend::maxSpritePalettes() const
{
    return mMaxSpritePalettes;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setMaxSpritePalettes(int maxSpritePalettes)
{
    mMaxSpritePalettes = maxSpritePalettes;
}

//---------------------------------------------------------------------------------------------------------------------

int OverlayPalGuiBackend::maxSpritesPerScanline() const
{
    return mMaxSpritesPerScanline;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setMaxSpritesPerScanline(int maxSpritesPerScanline)
{
    mMaxSpritesPerScanline = maxSpritesPerScanline;
}

//---------------------------------------------------------------------------------------------------------------------

int OverlayPalGuiBackend::timeOut() const
{
    return mTimeOut;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::setTimeOut(int timeOut)
{
    mTimeOut = timeOut;
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayPalGuiBackend::conversionSuccessful() const
{
    return mConversionError.size() == 0;
}

//---------------------------------------------------------------------------------------------------------------------

const QString &OverlayPalGuiBackend::conversionError() const
{
    return mConversionError;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::findOptimalShift()
{
    Image2D image = qImageToImage2D(mInputImageIndexedBeforeShift);
    int shiftX = 0;
    int shiftY = 0;
    Image2D shiftedImage = shiftImageOptimal(image, mBackgroundColor, mGridCellWidth, mGridCellHeight, 0, mGridCellWidth - 1, 0, mGridCellHeight - 1, shiftX, shiftY);
    if(shiftX != mShiftX || shiftY != mShiftY)
    {
        mShiftX = shiftX;
        mShiftY = shiftY;
        emit shiftXChanged();
        emit shiftYChanged();
        quantizeInputImage();
    }
}

//---------------------------------------------------------------------------------------------------------------------

QImage OverlayPalGuiBackend::shiftQImage(const QImage& qImage) const
{
    Image2D image = qImageToImage2D(qImage);
    Image2D shiftedImage = shiftImage(image, mShiftX, mShiftY);
    return image2DToQImage(shiftedImage, qImage.colorTable());
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::startImageConversion()
{
    // Prevent re-attempted conversion if multiple signals are emitted
    if(mConversionInProgress)
        return;
    mConversionInProgress = true;
    mImagePendingConversion = qImageToImage2D(mInputImageIndexed);

    // Start conversion in separate thread
    QFuture<void> future = QtConcurrent::run([=]()
    {
        try {
            std::string conversionError = mOverlayOptimiser.convert(mImagePendingConversion,
                                                                    mBackgroundColor,
                                                                    mGridCellWidth,
                                                                    mGridCellHeight,
                                                                    mSpriteHeight,
                                                                    GridCellColorLimit,
                                                                    mMaxBackgroundPalettes,
                                                                    mMaxSpritePalettes,
                                                                    mMaxSpritesPerScanline,
                                                                    mTimeOut);
            // successful
            QVector<QRgb> colorTable = makeColorTable();
            Image2D remappedImage = mOverlayOptimiser.outputImage();
            mOutputImage = image2DToQImage(remappedImage, colorTable);
            //
            const std::vector<std::set<uint8_t>>& palettes = mOverlayOptimiser.palettes();
            mPaletteModel.setPalette(palettes, mBackgroundColor);
            mConversionError = QString(conversionError.c_str());
            emit outputImageChanged();
        }
        catch (const std::runtime_error& error)
        {
            mConversionError = error.what();
            // Make dummy image
            QVector<QRgb> colorTable;
            colorTable.append(0x00000000);
            mOutputImage.fill(0);
            mOutputImage.setColorTable(colorTable);
            mOutputImageOverlay.fill(0);
            mOutputImageOverlay.setColorTable(colorTable);
            emit outputImageChanged();
        }
        mConversionInProgress = false;
    });
}

//---------------------------------------------------------------------------------------------------------------------

QVector<QRgb> OverlayPalGuiBackend::makeColorTable()
{
    const auto& rgbPalette = mHardwarePalettes[hardwarePaletteName()];
    const std::vector<std::set<uint8_t>>& palettes = mOverlayOptimiser.palettes();
    QVector<QRgb> colorTable;
    for(size_t i = 0; i < palettes.size(); i++)
    {
        // Background color
        uint8_t c = mBackgroundColor;
        QVariantList rgb = rgbPalette[c].toList();
        colorTable.append(qRgb(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt()));
        size_t j = 1;
        for(uint8_t c : palettes[i])
        {
            // Non-background color
            assert(c <= 0x3F && "NES palette value must be 0x00 - 0x3F");
            QVariantList rgb = rgbPalette[c].toList();
            colorTable.append(qRgb(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt()));
            j++;
        }
        // Fill unused entries
        while(j != PaletteGroupSize)
        {
            colorTable.append(0);
            j++;
        }
    }
    return colorTable;
}

//---------------------------------------------------------------------------------------------------------------------

QVector<QRgb> OverlayPalGuiBackend::makeColorTableFromHardwarePalette()
{
    const QVariantList& rgbPalette = mHardwarePalettes[hardwarePaletteName()];
    const std::vector<std::set<uint8_t>>& palettes = mOverlayOptimiser.palettes();
    QVector<QRgb> colorTable;
    for(size_t i = 0; i < HardwarePaletteSize; i++)
    {
        assert(i <= 0x3F && "NES palette value must be 0x00 - 0x3F");
        QVariantList rgb = rgbPalette[i].toList();
        colorTable.append(qRgb(rgb[0].toInt(), rgb[1].toInt(), rgb[2].toInt()));
    }
    return colorTable;
}

//---------------------------------------------------------------------------------------------------------------------

QString OverlayPalGuiBackend::inputImageData() const
{
    return imageAsBase64(mInputImageIndexed);
}

//---------------------------------------------------------------------------------------------------------------------

QImage OverlayPalGuiBackend::outputImage(int paletteMask) const
{
    const int w = mOutputImage.width();
    const int h = mOutputImage.height();
    QImage maskedImage(w, h, QImage::Format_Indexed8);
    maskedImage.setColorTable(mOutputImage.colorTable());
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            int pixelIndex = mOutputImage.pixelIndex(x, y);
            // Test against mask
            int palIndex = pixelIndex / PaletteGroupSize;
            if((1 << palIndex) & paletteMask)
            {
                maskedImage.setPixel(x, y, pixelIndex);
            }
            else
            {
                maskedImage.setPixel(x, y, 0);
            }
        }
    }
    return maskedImage;
}

//---------------------------------------------------------------------------------------------------------------------

QImage OverlayPalGuiBackend::outputImageRGBA(int paletteMask, bool transparentBG0) const
{
    QImage img = outputImage(paletteMask);
    const int w = img.width();
    const int h = img.height();
    QImage imgRGBA(w, h, QImage::Format_RGBA8888);
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            int pixelIndex = img.pixelIndex(x, y);
            // Test against mask
            if((pixelIndex % PaletteGroupSize == 0) && !transparentBG0)
            {
                QColor c = img.pixelColor(x, y);
                c.setAlpha(255);
                imgRGBA.setPixelColor(x, y, c);
            }
            else if(pixelIndex % PaletteGroupSize != 0)
            {
                // Indexed -> RGBA
                imgRGBA.setPixelColor(x, y, img.pixelColor(x, y));
            }
            else
            {
                // transparency
                imgRGBA.setPixelColor(x, y, QColor(0, 0, 0, 0));
            }
        }
    }
    return imgRGBA;
}

//---------------------------------------------------------------------------------------------------------------------

QString OverlayPalGuiBackend::imageAsBase64(const QImage& image)
{
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    // Encode as string data
    QString imageString("data:image/png;base64,");
    imageString.append(QString::fromLatin1(byteArray.toBase64().data()));
    return imageString;
}

//---------------------------------------------------------------------------------------------------------------------

QString OverlayPalGuiBackend::outputImageData(int paletteMask) const
{
    return imageAsBase64(outputImage(paletteMask));
}

//---------------------------------------------------------------------------------------------------------------------

QString OverlayPalGuiBackend::outputImageDataRGBA(int paletteMask, bool transparentBG0) const
{
    return imageAsBase64(outputImageRGBA(paletteMask, transparentBG0));
}

//---------------------------------------------------------------------------------------------------------------------

QObject *OverlayPalGuiBackend::paletteModel()
{
    return &mPaletteModel;
}

//---------------------------------------------------------------------------------------------------------------------

QObject *OverlayPalGuiBackend::hardwarePaletteNamesModel()
{
    return &mHardwarePaletteNamesModel;
}

//---------------------------------------------------------------------------------------------------------------------

uint8_t OverlayPalGuiBackend::findClosestColorIndex(const QVector<QRgb>& colorTable, QRgb rgb, std::vector<bool>& availableColors)
{
    QColor color(rgb);
    color.setAlpha(0);
    double bestDistance2 = std::numeric_limits<double>::max();
    size_t bestIndex = 0;
    for(size_t i = 0; i < colorTable.size(); i++)
    {
        QColor c(colorTable[i]);
        c.setAlpha(0);
        qreal dr = (color.redF() - c.redF());
        qreal dg = (color.greenF() - c.greenF());
        qreal db = (color.blueF() - c.blueF());
        double distance2 = dr * dr + dg * dg + db * db;
        if(distance2 < bestDistance2 && availableColors[i])
        {
            bestDistance2 = distance2;
            bestIndex = i;
        }
    }
    if(mUniqueColors)
        availableColors[bestIndex] = false;
    return static_cast<uint8_t>(bestIndex);
}

//---------------------------------------------------------------------------------------------------------------------

QImage OverlayPalGuiBackend::remapColorsToNES(const QImage& inputImage, uint8_t& backgroundColor)
{
    // Find all colors in image
    std::set<QRgb> colorsInImage;
    for(int y = 0; y < inputImage.height(); y++)
    {
        for(int x = 0; x < inputImage.width(); x++)
        {
            QRgb rgbColor = inputImage.pixel(x, y);
            colorsInImage.insert(rgbColor);
        }
    }
    // Remap every RGB color to color in chosen hardware palette
    QVector<QRgb> hwColorTable = makeColorTableFromHardwarePalette();
    std::unordered_map<QRgb, uint8_t> remapping;
    std::vector<bool> availableColors;
    availableColors.resize(HardwarePaletteSize, true);
    availableColors[0x0E] = false;
    availableColors[0x1E] = false;
    availableColors[0x2E] = false;
    availableColors[0x3E] = false;
    availableColors[0x0F] = false;
    availableColors[0x1F] = false;
    availableColors[0x2F] = false;
    availableColors[0x3F] = false;
    if(mPreventBlackerThanBlack)
        availableColors[0x0D] = false;
    for(QRgb rgbColor : colorsInImage)
    {
        if(!colorsInImage.count(rgbColor) > 0)
            continue;
        uint8_t c = findClosestColorIndex(hwColorTable, rgbColor, availableColors);
        remapping[rgbColor] = c;
    }
    QImage outputImage(inputImage.width(), inputImage.height(), QImage::Format_Indexed8);
    outputImage.setColorTable(hwColorTable);
    // Remap background color
    backgroundColor = remapping[backgroundColor];
    // Remap  image
    for(int y = 0; y < inputImage.height(); y++)
    {
        for(int x = 0; x < inputImage.width(); x++)
        {
            QRgb rgbColor = inputImage.pixel(x, y);
            uint8_t c = remapping[rgbColor];
            outputImage.setPixel(x, y, c);
        }
    }
    return outputImage;
}

//---------------------------------------------------------------------------------------------------------------------

Q_INVOKABLE QVariantList OverlayPalGuiBackend::hardwarePaletteRGB() const
{
    return mHardwarePalettes[mHardwarePaletteName];
}

//---------------------------------------------------------------------------------------------------------------------

Q_INVOKABLE QObject* OverlayPalGuiBackend::inputImageColorsModel()
{
    return &mInputImageHardwareColorsModel;
}

//---------------------------------------------------------------------------------------------------------------------

QVariantList OverlayPalGuiBackend::debugPaletteIndices(const Array2D<uint8_t>& paletteIndices) const
{
    QVariantList paletteIndicesQML;
    for(size_t y = 0; y < paletteIndices.height(); y++)
    {
        QVariantList paletteIndicesRowQML;
        for(size_t x = 0; x < paletteIndices.width(); x++)
        {
            uint8_t paletteIndex = paletteIndices(x, y);
            paletteIndicesRowQML.push_back(QString::number(paletteIndex));
        }
        paletteIndicesQML.push_back(paletteIndicesRowQML);
    }
    return paletteIndicesQML;
}

//---------------------------------------------------------------------------------------------------------------------

QVariantList OverlayPalGuiBackend::debugNumSourceColors(const GridLayer& layer) const
{
    QVariantList numSourceColorsQML;
    for(size_t y = 0; y < layer.height(); y++)
    {
        QVariantList numSourceColorsRowQML;
        for(size_t x = 0; x < layer.width(); x++)
        {
            size_t numSourceColors = layer(x, y).colors.size();
            numSourceColorsRowQML.push_back(QString::number(numSourceColors));
        }
        numSourceColorsQML.push_back(numSourceColorsRowQML);
    }
    return numSourceColorsQML;
}

//---------------------------------------------------------------------------------------------------------------------

QString colorIndexToQString(const std::vector<uint8_t>& colors, size_t i)
{
    if(i < colors.size())
    {
        return QString("%1").arg(int(colors[i]), 2, 16, QChar('0')).toUpper();
    }
    else
    {
        return QString("..");
    }
}

//---------------------------------------------------------------------------------------------------------------------

QString colorsToQString(const std::vector<uint8_t>& colors, int valuesPerLine = 2)
{
    // Print first 4 colors into 4 quadrants
    QString s;
    for(size_t i = 0; i < 4; i++)
    {
        s += colorIndexToQString(colors, i);
        s += (((i + 1) % valuesPerLine) == 0) ? QString("\n") : QString("");
    }
    return s;
}

//---------------------------------------------------------------------------------------------------------------------

QVariantList OverlayPalGuiBackend::debugColors(const GridLayer& layer,
                                               const Array2D<uint8_t>& paletteIndices,
                                               bool remapped) const
{
    QVariantList sourceColorsQML;
    for(size_t y = 0; y < layer.height(); y++)
    {
        QVariantList sourceColorsRowQML;
        for(size_t x = 0; x < layer.width(); x++)
        {
            std::vector<uint8_t> colors;
            uint8_t paletteIndex = paletteIndices(x, y);
            size_t i = 1;
            for(uint8_t c : layer(x, y).colors)
            {
                if(colors.size() < 4)
                {
                    uint8_t cRemapped = (PaletteGroupSize * paletteIndex) | i;
                    colors.push_back(remapped ? cRemapped : c);
                }
                i++;
            }
            sourceColorsRowQML.push_back(colorsToQString(colors));
        }
        sourceColorsQML.push_back(sourceColorsRowQML);
    }
    return sourceColorsQML;
}

//---------------------------------------------------------------------------------------------------------------------

QVariantList OverlayPalGuiBackend::debugPaletteIndicesBackground() const
{
    const Array2D<uint8_t>& paletteIndices = mOverlayOptimiser.debugPaletteIndicesBackground();
    return debugPaletteIndices(paletteIndices);
}

//---------------------------------------------------------------------------------------------------------------------

QVariantList OverlayPalGuiBackend::debugNumSourceColorsBackground() const
{
    const GridLayer& layer = mOverlayOptimiser.layerBackground();
    return debugNumSourceColors(layer);
}

//---------------------------------------------------------------------------------------------------------------------

QVariantList OverlayPalGuiBackend::debugSourceColorsBackground() const
{
    const GridLayer& layer = mOverlayOptimiser.layerBackground();
    return debugColors(layer, mOverlayOptimiser.debugPaletteIndicesBackground(), false);
}

//---------------------------------------------------------------------------------------------------------------------

QVariantList OverlayPalGuiBackend::debugDestinationColorsBackground() const
{
    const GridLayer& layer = mOverlayOptimiser.layerBackground();
    return debugColors(layer, mOverlayOptimiser.debugPaletteIndicesBackground(), true);
}

//---------------------------------------------------------------------------------------------------------------------

QString OverlayPalGuiBackend::urlToLocal(const QString &url)
{
    return QUrl(url).toLocalFile();
}

//---------------------------------------------------------------------------------------------------------------------

QVariantList OverlayPalGuiBackend::debugSpritesOverlay() const
{
    const std::vector<std::set<uint8_t>>& palettes = mOverlayOptimiser.palettes();
    std::vector<Sprite> sprites = mOverlayOptimiser.spritesOverlay();
    QVariantList spritesQML;
    for(auto& s : sprites)
    {
        QVariantMap m;
        m["x"] = s.x;
        m["y"] = s.y;
        // Make palette start at 0 to match "SPR0" designation
        m["p"] = s.p - 4;
        m["numColors"] = int(s.colors.size());
        // Get current sprite width / height from optimiser
        m["w"] = mOverlayOptimiser.spriteWidth();
        m["h"] = mOverlayOptimiser.spriteHeight();
        std::vector<uint8_t> srcColors;
        std::vector<uint8_t> dstColors;
        uint8_t i = 1;
        for(uint8_t c : s.colors)
        {
            srcColors.push_back(c);
            uint8_t dstColor = mOverlayOptimiser.indexInPalette(palettes[s.p], c);
            dstColors.push_back((s.p << 2) | dstColor);
            i++;
        }
        int valuesPerLine = (mOverlayOptimiser.spriteHeight() == 8) ? 2 : 1;
        m["srcColors"] = colorsToQString(srcColors, valuesPerLine);
        m["dstColors"] = colorsToQString(dstColors, valuesPerLine);
        spritesQML.push_back(m);
    }
    return spritesQML;
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::saveOutputImage(QString filename, int paletteMask)
{
    QImage img = outputImage(paletteMask);
    filename = urlToLocal(filename);
    img.save(filename);
}

//---------------------------------------------------------------------------------------------------------------------

void OverlayPalGuiBackend::exportOutputImage(QString filename, int paletteMask)
{
    filename = urlToLocal(filename);
    QFileInfo fi(filename);
    ExportDataNES exportData = buildExportData(mOverlayOptimiser, paletteMask);
    // Create filename suffixes based on selected .nam file
    QString nametableFilename = fi.path() + "/" + fi.baseName() + ".nam";
    QString exramFilename = fi.path() + "/" + fi.baseName() + ".exram";
    QString bgCHRFilename = fi.path() + "/" + fi.baseName() + "_bg.chr";
    QString sprCHRFilename = fi.path() + "/" + fi.baseName() + "_spr.chr";
    QString oamFilename = fi.path() + "/" + fi.baseName() + ".oam";
    QString paletteFilename = fi.path() + "/" + fi.baseName() + "_palette.dat";
    // Write all binary data to files
    writeBinaryFile(nametableFilename, exportData.nametable);
    writeBinaryFile(exramFilename, exportData.exram);
    writeBinaryFile(bgCHRFilename, exportData.bgCHR);
    writeBinaryFile(oamFilename, exportData.oam);
    writeBinaryFile(sprCHRFilename, exportData.oamCHR);
    writeBinaryFile(paletteFilename, exportData.palette);
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayPalGuiBackend::writeBinaryFile(QString filename, const QByteArray& a)
{
    QFile file(filename);
    if(!file.open(QFile::WriteOnly))
        return false;
    file.write(a);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayPalGuiBackend::writeBinaryFile(const QString& filename, const std::vector<uint8_t>& v)
{
    QByteArray a;
    for(uint8_t b : v)
    {
        a.append(b);
    }
    return writeBinaryFile(filename, a);
}

//---------------------------------------------------------------------------------------------------------------------

int OverlayPalGuiBackend::numBackgroundTiles() const
{
    ExportDataNES exportData = buildExportData(mOverlayOptimiser, 0xFF);
    return exportData.bgCHR.size() / ExportDataNES::TileSize;
}
