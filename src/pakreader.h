#pragma once

#include <QString>
#include <QImage>
#include <QMetaType>
#include <vector>
#include <memory>

struct SpriteEntry {
    QString name;
    QByteArray data;
    QImage image;
    
    bool loadImage();
    bool exportTo(const QString& path, const QString& format) const;
    QImage preview() const { return image; }
};

Q_DECLARE_METATYPE(std::shared_ptr<SpriteEntry>)

class PakReader {
public:
    explicit PakReader();
    
    bool readFile(const QString& path);
    std::vector<std::shared_ptr<SpriteEntry>> entries() const;

private:
    bool tryFormat1(const QByteArray& data);  // Standard PAK format
    bool tryFormat2(const QByteArray& data);  // Alternative format
    std::vector<std::shared_ptr<SpriteEntry>> m_entries;
}; 