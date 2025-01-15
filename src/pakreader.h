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
    bool tryFormat1(const QByteArray& data);  // PNG/BMP sequence format
    bool tryFormat2(const QByteArray& data);  // Size + data chunks format
    bool tryFormat3(const QByteArray& data);  // Scan for image signatures
    int findPNGEnd(const QByteArray& data, int start);
    std::vector<std::shared_ptr<SpriteEntry>> m_entries;
}; 