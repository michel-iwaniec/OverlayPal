#include "OverlayPalApp.h"
#include <QStandardPaths>
#include <QDir>

OverlayPalApp::OverlayPalApp(int &argc, char **argv)
    : QGuiApplication(argc, argv)
{
    setOrganizationName("RTC");
    setOrganizationDomain("nodomain");
    initFS();
}

QString OverlayPalApp::appStoragePath(const QString & path) const
{
    if (appDataFolder.isEmpty()) {
        return appDataFolder;
    } else {
        if (path.isEmpty()) {
            return appDataFolder;  // if empty you're fucked mate
        } else {
            // does not mkdir if it doesn't exist - please do this
            // then you can merge the code up above
            return appDataFolder + QDir::separator() + path;
        }
    }
}

bool OverlayPalApp::initFS()
{
    QDir dirAppData(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dirAppData.exists()) {
        // required on macOS, possibly iOS. kinda weird
        if (dirAppData.mkpath(".")) {
            appDataFolder = dirAppData.path();
        } else {
            return false;
        }
    } else {
        appDataFolder = dirAppData.path();
    }
    return true;

}
