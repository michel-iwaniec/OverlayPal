#include <QStandardPaths>
#include <QDir>

#include "OverlayPalApp.h"

//---------------------------------------------------------------------------------------------------------------------

OverlayPalApp::OverlayPalApp(int argc, char **argv)
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
