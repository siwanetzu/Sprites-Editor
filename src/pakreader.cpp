#include "pakreader.h"
#include <QFile>
#include <QDataStream>
#include <QBuffer>
#include <QDebug>

PakReader::PakReader() {}

bool PakReader::readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file:" << path;
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    try {
        // Read the entire file content
        QByteArray fileData = file.readAll();
        qDebug() << "File size:" << fileData.size() << "bytes";

        // Try different PAK formats
        if (!tryFormat1(fileData) && !tryFormat2(fileData)) {
            qDebug() << "Failed to parse PAK file in any known format";
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        qDebug() << "Exception while reading PAK file:" << e.what();
        return false;
    }
}

bool PakReader::tryFormat1(const QByteArray& data) {
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QBuffer::ReadOnly);
    QDataStream stream(&buffer);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Try reading as format 1 (standard PAK)
    char header[4];
    stream.readRawData(header, 4);
    
    if (memcmp(header, "PACK", 4) != 0) {
        return false;
    }

    quint32 fileSize, numEntries;
    stream >> fileSize >> numEntries;

    if (numEntries > 10000) { // Sanity check
        return false;
    }

    m_entries.clear();
    for (quint32 i = 0; i < numEntries; ++i) {
        quint32 offset, size;
        quint8 nameLength;
        stream >> offset >> size >> nameLength;

        if (nameLength > 100) continue; // Skip invalid entries

        QByteArray nameData = buffer.read(nameLength);
        QString name = QString::fromUtf8(nameData);

        auto entry = std::make_shared<SpriteEntry>();
        entry->name = name;

        // Read sprite data
        qint64 currentPos = buffer.pos();
        buffer.seek(offset);
        entry->data = buffer.read(size);
        buffer.seek(currentPos);

        if (entry->loadImage()) {
            m_entries.push_back(entry);
            qDebug() << "Loaded sprite:" << name;
        }
    }

    return !m_entries.empty();
}

bool PakReader::tryFormat2(const QByteArray& data) {
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QBuffer::ReadOnly);
    QDataStream stream(&buffer);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Try reading as format 2 (alternative format)
    quint32 numEntries;
    stream >> numEntries;

    if (numEntries > 10000) return false;

    m_entries.clear();
    qint64 currentOffset = 4; // Start after numEntries

    for (quint32 i = 0; i < numEntries; ++i) {
        quint32 size;
        stream >> size;
        
        if (size > buffer.size() - currentOffset) break;

        auto entry = std::make_shared<SpriteEntry>();
        entry->name = QString("sprite_%1").arg(i);
        entry->data = buffer.read(size);
        currentOffset += 4 + size;

        if (entry->loadImage()) {
            m_entries.push_back(entry);
            qDebug() << "Loaded sprite:" << entry->name;
        }
    }

    return !m_entries.empty();
}

bool SpriteEntry::loadImage() {
    try {
        QBuffer buffer(&data);
        buffer.open(QBuffer::ReadOnly);
        
        // Try different image formats
        if (image.load(&buffer, "PNG")) return true;
        
        buffer.seek(0);
        if (image.load(&buffer, "BMP")) return true;
        
        buffer.seek(0);
        if (image.load(&buffer, "JPG")) return true;
        
        buffer.seek(0);
        if (image.load(&buffer, "GIF")) return true;

        // Try raw image data
        buffer.seek(0);
        QImage img(reinterpret_cast<const uchar*>(data.constData()), 
                  32, 32, QImage::Format_ARGB32);  // Try common sprite size
        if (!img.isNull()) {
            image = img;
            return true;
        }

        return false;
    } catch (...) {
        qDebug() << "Exception while loading image";
        return false;
    }
}

std::vector<std::shared_ptr<SpriteEntry>> PakReader::entries() const {
    return m_entries;
}

bool SpriteEntry::exportTo(const QString& path, const QString& format) const {
    try {
        if (!image.isNull()) {
            return image.save(path, format.toUtf8().constData());
        }
        
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        return file.write(data) == data.size();
    } catch (...) {
        qDebug() << "Exception while exporting sprite";
        return false;
    }
} 