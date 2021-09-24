#ifndef HARDWARECOLORSMODEL_H
#define HARDWARECOLORSMODEL_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QAbstractListModel>

#include <cstdint>
#include <cassert>
#include <vector>
#include <set>

//
// Model used for providing list of hardware colors along with appropriate coloring
//
class HardwareColorsModel : public QAbstractListModel
{
    Q_OBJECT

public:

    HardwareColorsModel();

    ~HardwareColorsModel() override;

    void setColors(const std::vector<uint8_t>& hardwareColors);

    void setColors(const std::set<uint8_t>& hardwareColors);

    void setHardwarePalette(const QVariantList& hardwarePalette);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QHash<int, QByteArray> roleNames() const override;

protected:

    float luminance(int r, int g, int b) const
    {
        return 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }

    QColor foregroundTextColor(QColor backgroundColor) const
    {
        if(luminance(backgroundColor.red(), backgroundColor.green(), backgroundColor.blue()) < 128.0f)
        {
            // Pick white foreground color
            return QColor(255, 255, 255);
        }
        else
        {
            // Pick black foreground color
            return QColor(0, 0, 0);
        }
    }

private:
    std::vector<uint8_t> mColors;
    QVariantList mHardwarePalette;
};

#endif // HARDWARECOLORSMODEL_H
