#pragma once

#include <QString>
#include <QImage>
#include <QMetaType>
#include <vector>
#include <memory>

// Forward declare SpriteEntry
struct SpriteEntry;

// Declare metatype before the struct definition
Q_DECLARE_METATYPE(std::shared_ptr<SpriteEntry>);

struct SpriteEntry {
    QString name;
    QByteArray data;
    QImage image;
    
    bool loadImage();
    bool exportTo(const QString& path, const QString& format) const;
    QImage preview() const { return image; }
};

class PakReader {
public:
    explicit PakReader();
    
    bool readFile(const QString& path);
    std::vector<std::shared_ptr<SpriteEntry>> entries() const;

private:
    std::vector<std::shared_ptr<SpriteEntry>> m_entries;
}; 