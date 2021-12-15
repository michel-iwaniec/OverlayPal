#pragma once
#ifndef OVERLAYPALAPP_H
#define OVERLAYPALAPP_H

#include <QGuiApplication>

class OverlayPalApp : public QGuiApplication
{
    Q_OBJECT
public:
    OverlayPalApp(int argc, char **argv);

    QString appStoragePath(const QString& path = QString()) const;
private:
    bool initFS();
    //
    QString appDataFolder;
};

#undef qApp
#define qApp qobject_cast<OverlayPalApp *>(QCoreApplication::instance())

#endif // OVERLAYPALAPP_H
