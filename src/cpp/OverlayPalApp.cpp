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

#include <QStandardPaths>
#include <QDir>

#include "OverlayPalApp.h"

//---------------------------------------------------------------------------------------------------------------------

OverlayPalApp::OverlayPalApp(int& argc, char **argv)
    : QGuiApplication(argc, argv)
{
    setOrganizationName("RTC");
    setOrganizationDomain("nodomain");
    initFS();
}

//---------------------------------------------------------------------------------------------------------------------

QString OverlayPalApp::appStoragePath(const QString& path) const
{
    if(appDataFolder.isEmpty())
    {
        return appDataFolder;
    }
    else
    {
        if(path.isEmpty())
        {
            return appDataFolder;
        }
        else
        {
            // does not mkdir if it doesn't exist - please do this
            // then you can merge the code up above
            return appDataFolder + QDir::separator() + path;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool OverlayPalApp::initFS()
{
    QDir dirAppData(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if(!dirAppData.exists())
    {
        // required on macOS, and possibly iOS.
        if(dirAppData.mkpath("."))
        {
            appDataFolder = dirAppData.path();
        }
        else
        {
            return false;
        }
    }
    else
    {
        appDataFolder = dirAppData.path();
    }
    return true;
}
