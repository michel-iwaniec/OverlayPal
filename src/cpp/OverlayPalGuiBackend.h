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
#ifndef OVERLAYPAL_GUI_BACKEND_H
#define OVERLAYPAL_GUI_BACKEND_H

#include <QObject>
#include <QString>
#include <qqml.h>
#include <QtQml>
#include <QImage>
#include <QBrush>
#include <QVector>
#include <QRgb>
#include <QFileSystemWatcher>

#include "Array2D.h"
#include "OverlayOptimiser.h"
#include "SimplePaletteModel.h"
#include "HardwareColorsModel.h"

class OverlayPalGuiBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool trackInputImage READ trackInputImage WRITE setTrackInputImage)
    Q_PROPERTY(bool potentialHardwarePaletteIndexedImage READ potentialHardwarePaletteIndexedImage)
    Q_PROPERTY(bool mapInputColors READ mapInputColors WRITE setMapInputColors NOTIFY mapInputColorsChanged)
    Q_PROPERTY(bool uniqueColors READ uniqueColors WRITE setUniqueColors)
    Q_PROPERTY(int backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QString inputImageFilename READ inputImageFilename WRITE setInputImageFilename)
    Q_PROPERTY(int shiftX READ shiftX WRITE setShiftX NOTIFY shiftXChanged)
    Q_PROPERTY(int shiftY READ shiftY WRITE setShiftY NOTIFY shiftYChanged)
    Q_PROPERTY(QSize cellSize READ cellSize WRITE setCellSize)
    Q_PROPERTY(int spriteHeight READ spriteHeight WRITE setSpriteHeight)
    Q_PROPERTY(int maxBackgroundPalettes READ maxBackgroundPalettes WRITE setMaxBackgroundPalettes)
    Q_PROPERTY(int maxSpritePalettes READ maxSpritePalettes WRITE setMaxSpritePalettes)
    Q_PROPERTY(int maxSpritesPerScanline READ maxSpritesPerScanline WRITE setMaxSpritesPerScanline)
    Q_PROPERTY(int timeOut READ timeOut WRITE setTimeOut)
    Q_PROPERTY(QString hardwarePaletteName READ hardwarePaletteName WRITE setHardwarePaletteName)
    Q_PROPERTY(bool conversionSuccessful READ conversionSuccessful)
    Q_PROPERTY(QString conversionError READ conversionError)

public:
    explicit OverlayPalGuiBackend(QObject *parent = nullptr);
    ~OverlayPalGuiBackend() override;

    QString inputImageFilename() const;
    void setInputImageFilename(const QString& inputImageFilename);

    int backgroundColor() const;
    void setBackgroundColor(int backgroundColor);

    bool trackInputImage() const;
    void setTrackInputImage(bool trackInputImage);

    int shiftX() const;
    void setShiftX(const int &shiftX);
    int shiftY() const;
    void setShiftY(const int &shiftY);

    bool potentialHardwarePaletteIndexedImage() const;
    bool mapInputColors() const;
    void setMapInputColors(bool mapInputColors);
    bool uniqueColors() const;
    void setUniqueColors(bool uniqueColors);
    QSize cellSize() const;
    void setCellSize(QSize cellSize);
    int spriteHeight() const;
    void setSpriteHeight(int spriteHeight);
    int maxBackgroundPalettes() const;
    void setMaxBackgroundPalettes(int maxBackgroundPalettes);
    int maxSpritePalettes() const;
    void setMaxSpritePalettes(int maxSpritePalettes);
    int maxSpritesPerScanline() const;
    void setMaxSpritesPerScanline(int maxSpritesPerScanline);

    int timeOut() const;
    void setTimeOut(int timeOut);

    bool conversionSuccessful() const;

    const QString& conversionError() const;

    static QString imageAsBase64(const QImage& image);

    Q_INVOKABLE QString inputImageData() const;
    Q_INVOKABLE QImage outputImage(int paletteMask) const;
    Q_INVOKABLE QImage outputImageRGBA(int paletteMask, bool transparentBG0) const;
    Q_INVOKABLE QString outputImageData(int paletteMask) const;
    Q_INVOKABLE QString outputImageDataRGBA(int paletteMask, bool transparentBG0) const;

    Q_INVOKABLE QObject* paletteModel();

    Q_INVOKABLE QObject* hardwarePaletteNamesModel();

    Q_INVOKABLE QVariantList hardwarePaletteRGB() const;

    Q_INVOKABLE QObject* inputImageColorsModel();
    Q_INVOKABLE QVariantList debugPaletteIndicesBackground() const;
    Q_INVOKABLE QVariantList debugNumSourceColorsBackground() const;
    Q_INVOKABLE QVariantList debugSourceColorsBackground() const;
    Q_INVOKABLE QVariantList debugDestinationColorsBackground() const;
    Q_INVOKABLE QVariantList debugSpritesOverlay() const;

    Q_INVOKABLE void saveOutputImage(QString filename, int paletteMask);


public slots:
    void findOptimalShift();

    QImage shiftQImage(const QImage& qImage) const;

    void startImageConversion();

    void handleInputFileChanged(const QString& filename);

    void quantizeInputImage();

signals:
    void mapInputColorsChanged();
    void backgroundColorChanged();
    void shiftXChanged();
    void shiftYChanged();

    void inputImageChanged();
    void outputImageChanged();

protected:

    QVariantList debugPaletteIndices(const Array2D<uint8_t>& paletteIndices) const;
    QVariantList debugNumSourceColors(const GridLayer& layer) const;
    QVariantList debugColors(const GridLayer& layer,
                             const Array2D<uint8_t>& paletteIndices,
                             bool remapped) const;

    static Image2D qImageToImage2D(const QImage& qImage);
    static QImage image2DToQImage(const Image2D& image, const QVector<QRgb>& colorTable);

    QVector<QRgb> makeColorTable();
    QVector<QRgb> makeColorTableFromHardwarePalette();

    const QString& hardwarePaletteName() const;
    void setHardwarePaletteName(const QString& hardwarePaletteName);
    void loadHardwarePalette(const QFileInfo& fileInfo);
    void loadHardwarePalettes(const QString& palettesPath);
    uint8_t findClosestColorIndex(const QVector<QRgb>& colorTable, QRgb rgb, std::vector<bool>& availableColors);
    QImage remapColorsToNES(const QImage& inputImage, uint8_t& backgroundColor);

    static QImage cropOrExtendImage(const QImage& image, uint8_t backgroundColor);
    static uint8_t detectBackgroundColor(const QImage& image, uint8_t oldBackgroundColor);

    static uint8_t indexInPalette(const std::set<uint8_t>& palette, uint8_t color);

    static QString urlToLocal(const QString& url);

private:
    bool mUniqueColors;
    int mTimeOut;
    bool mTrackInputImage;
    int mShiftX;
    int mShiftY;
    int mSpriteHeight;
    int mMaxBackgroundPalettes;
    int mMaxSpritePalettes;
    int mMaxSpritesPerScanline;
    bool mMapInputColors;
    uint8_t mBackgroundColor;
    bool mPreventBlackerThanBlack;
    bool mInputImagePaletteMapping;
    bool mConversionInProgress;
    bool mProcessingInputImage;
    QString mConversionError;
    QString mHardwarePaletteName;
    QString mInputImageFilename;
    QImage mInputImage;
    QImage mInputImageIndexedBeforeShift;
    QImage mInputImageIndexed;
    QImage mOutputImage;
    QImage mOutputImageOverlay;
    Image2D mImagePendingConversion;
    QMap<QString, QVariantList> mHardwarePalettes;
    QStringList mHardwarePaletteNames;
    QStringListModel mHardwarePaletteNamesModel;
    SimplePaletteModel mPaletteModel;
    HardwareColorsModel mInputImageHardwareColorsModel;

    OverlayOptimiser mOverlayOptimiser;
    QFileSystemWatcher mInputFileWatcher;

    const size_t PaletteGroupSize = 4;
    // 16x16 or 8x8 Nametable grid
    int mGridCellWidth;
    int mGridCellHeight;
    const int GridCellColorLimit = 3;
    static const size_t HardwarePaletteSize = 64;
    static const int ScreenWidth = 256;
    static const int ScreenHeight = 240;
};

#endif // OVERLAYPAL_GUI_BACKEND_H
