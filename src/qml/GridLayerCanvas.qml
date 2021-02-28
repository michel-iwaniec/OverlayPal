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

import QtQuick 2.12
import QtQuick.Window 2.3
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls.Universal 2.0
import QtQuick.Extras 1.4
import QtQuick.Dialogs 1.0

import "const.js" as Const

Item {
    id: gridLayerCanvas
    anchors.fill: parent
    width: 768
    height: 720
    smooth: false
    property bool showGrid: true
    property int gridCellWidth: 16
    property int gridCellHeight: 16
    property int gridWidth: 16
    property int gridHeight: 15
    property int zoom: 3
    property string cellDebugMode: 'off'
    property bool spriteDebugMode: false
    property var paletteGroupImages: ({})
    property var backdropImage: null
    property var showPaletteGroup: [];
    property var debugNumSourceColorsBackground: [];
    property var debugSourceColorsBackground: [];
    property var debugDestinationColorsBackground: [];
    property var debugPaletteIndicesBackground: [];
    property var debugSprites: [];
    property var debugColor: "green";
    z: 2
    visible: true

    Canvas {
        id: canvas
        anchors.fill: parent
        width: 768
        height: 720
        smooth: false

        onPaint: {
            var ctx = getContext("2d");
            paintOnCanvas(ctx);
        }
    }

    Component.onCompleted: {
        for(var i = 0; i < (Const.NumPaletteGroupsBG + Const.NumPaletteGroupsSPR); i++)
        {
            showPaletteGroup.push(true);
        }
    }

    function inputImageUpdated()
    {
        if(backdropImage)
        {
            createImageLoaderProxy(backdropImage);
        }
        for(var i = 0; i < showPaletteGroup.length; i++)
        {
            if(i in paletteGroupImages)
            {
                var img = paletteGroupImages[i];
                if(img)
                {
                    createImageLoaderProxy(img);
                }
            }
        }
    }

    function createImageLoaderProxy(img)
    {
        var component = Qt.createComponent("ImageLoadingProxy.qml");
        var proxy = component.createObject(gridLayerCanvas, { "imageCanvas": canvas });
        proxy.setImage(canvas, img);
        return proxy;
    }

    function requestPaint()
    {
        canvas.requestPaint();
    }

    onCellDebugModeChanged: {
        canvas.requestPaint()
    }

    onSpriteDebugModeChanged: {
        canvas.requestPaint()
    }

    function paintOnCanvas(ctx)
    {
        ctx.reset();
        // Draw backdrop
        if(backdropImage)
        {
            ctx.drawImage(backdropImage, 0, 0, 768, 720);
        }
        // Draw each palette layer
        for(var i = 0; i < showPaletteGroup.length; i++)
        {
            if(showPaletteGroup[i])
            {
                var img = paletteGroupImages[i]
                if(img)
                {
                    ctx.drawImage(img, 0, 0, 768, 720);
                }
            }
        }
        if(showGrid && !spriteDebugMode) {
            drawGrid(ctx);
        }
        // Draw debug text
        if(spriteDebugMode)
        {
            switch(cellDebugMode)
            {
                case 'numSrcColors':
                    drawDebugTextSpr(ctx, debugSprites, "numColors", false);
                    break;
                case 'srcColors':
                    drawDebugTextSpr(ctx, debugSprites, "srcColors", true);
                    break;
                case 'dstColors':
                    drawDebugTextSpr(ctx, debugSprites, "dstColors", true);
                    break;
                case 'paletteIndex':
                    drawDebugTextSpr(ctx, debugSprites, "p", false);
                    break;
                default:
                    break;
            }
        }
        else
        {
            switch(cellDebugMode)
            {
                case 'numSrcColors':
                    drawDebugText(ctx, debugNumSourceColorsBackground, false, false);
                    break;
                case 'srcColors':
                    drawDebugText(ctx, debugSourceColorsBackground, true, false);
                    break;
                case 'dstColors':
                    drawDebugText(ctx, debugDestinationColorsBackground, true, false);
                    break;
                case 'paletteIndex':
                    drawDebugText(ctx, debugPaletteIndicesBackground, false, false);
                    break;
                default:
                    break;
            }
        }
    }

    function drawGrid(ctx)
    {
        ctx.lineWidth = 1;
        ctx.strokeStyle = debugColor;
        for(var i = 0; i < gridHeight; i++)
        {
            for(var j = 0; j < gridWidth; j++)
            {
                var x = j * gridCellWidth;
                var y = i * gridCellHeight;
                ctx.rect(x * zoom + 0.5,
                         y * zoom + 0.5,
                         gridCellWidth * zoom,
                         gridCellHeight * zoom);
            }
        }
        ctx.stroke();
    }

    function drawDebugText(ctx, perCellText, useQuadrants, useRows)
    {
        var qScale = useQuadrants ? 0.4 : 1.0;
        var gridCellSize = Math.min(gridCellWidth, gridCellHeight);
        ctx.fillStyle = debugColor;
        ctx.font = (zoom * gridCellSize * qScale) + 'px sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'bottom'
        for(var i = 0; i < gridHeight; i++)
        {
            for(var j = 0; j < gridWidth; j++)
            {
                var x = (j + 0.5) * gridCellWidth;
                var y = (i + qScale) * gridCellHeight;
                if( i < perCellText.length && j < perCellText[0].length )
                {
                    if(useRows)
                    {
                        drawDebugTextImplRows(ctx, x, y, gridCellHeight, perCellText[i][j]);
                    }
                    else
                    {
                        drawDebugTextImpl(ctx, x, y, gridCellHeight, perCellText[i][j], useQuadrants);
                    }
                }
            }
        }
    }

    function drawDebugTextImpl(ctx, x, y, gridCellHeight, cellText, useQuadrants)
    {
        if( useQuadrants )
        {
            // Split into upper / lower pair
            var pairs = cellText.split('\n');
            if( pairs.length > 0)
            {
                ctx.fillText(pairs[0], x * zoom, y * zoom);
            }
            if( pairs.length > 1)
            {
                ctx.fillText(pairs[1], x * zoom, (y + 0.5 * gridCellHeight) * zoom);
            }
        }
        else
        {
            ctx.fillText(cellText, x * zoom, y * zoom);
        }
    }

    function drawDebugTextImplRows(ctx, x, y, gridCellHeight, cellText)
    {
        // Split into rows
        var colorsStr = cellText.split('\n');
        for(var i = 0; i < colorsStr.length; i++)
        {
            ctx.fillText(colorsStr[i], x * zoom, (y + 0.2 * (i - 1) * gridCellHeight) * zoom);
        }
    }

    function drawDebugTextSpr(ctx, sprites, keyword, useRows)
    {
        var qScale = useRows ? 0.5 : 1.0;
        var gridCellWidth = 8;
        var gridCellHeight = 16;
        ctx.fillStyle = debugColor;
        ctx.font = (zoom * gridCellWidth * qScale) + 'px sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'bottom';
        ctx.lineWidth = 1;
        ctx.strokeStyle = debugColor;
        for(var i = 0; i < sprites.length; i++)
        {
            var s = sprites[i];
            var x = s.x + 0.5 * gridCellWidth;
            var y = s.y + qScale * gridCellHeight;
            ctx.rect(s.x * zoom + 0.5,
                     s.y * zoom + 0.5,
                     gridCellWidth * zoom,
                     gridCellHeight * zoom);
            ctx.stroke();
            if(useRows)
            {
                drawDebugTextImplRows(ctx, x, y, gridCellHeight, s[keyword]);
            }
            else
            {
                drawDebugTextImpl(ctx, x, y, gridCellHeight, s[keyword], false, useRows);
            }
        }
    }

    function toggleGrid() {
        showGrid = !showGrid;
        requestPaint();
    }

}
