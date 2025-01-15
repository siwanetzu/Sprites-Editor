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
        qDebug() << "First 16 bytes:" << fileData.left(16).toHex();

        // Try different PAK formats
        if (!tryFormat1(fileData) && !tryFormat2(fileData) && !tryFormat3(fileData)) {
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
    qDebug() << "Trying Format 1...";
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QBuffer::ReadOnly);

    // Try to read as a sequence of PNG/BMP files
    m_entries.clear();
    int offset = 0;

    while (offset < data.size()) {
        buffer.seek(offset);
        QByteArray header = buffer.peek(8);
        
        // Check for PNG signature
        if (header.startsWith(QByteArray::fromHex("89504E470D0A1A0A"))) {
            qDebug() << "Found PNG signature at offset" << offset;
            auto entry = std::make_shared<SpriteEntry>();
            entry->name = QString("sprite_%1").arg(m_entries.size());
            
            // Find PNG end marker
            int size = findPNGEnd(data, offset);
            if (size > 0) {
                entry->data = data.mid(offset, size);
                if (entry->loadImage()) {
                    m_entries.push_back(entry);
                    offset += size;
                    continue;
                }
            }
        }
        
        // Move to next byte
        offset++;
    }

    if (!m_entries.empty()) {
        qDebug() << "Format 1 succeeded with" << m_entries.size() << "entries";
        return true;
    }
    return false;
}

bool PakReader::tryFormat2(const QByteArray& data) {
    qDebug() << "Trying Format 2...";
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QBuffer::ReadOnly);
    QDataStream stream(&buffer);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Try reading as a simple format with size + data chunks
    m_entries.clear();
    while (!buffer.atEnd()) {
        quint32 size;
        stream >> size;
        
        if (size > buffer.bytesAvailable() || size == 0) {
            qDebug() << "Invalid chunk size:" << size;
            break;
        }

        auto entry = std::make_shared<SpriteEntry>();
        entry->name = QString("sprite_%1").arg(m_entries.size());
        entry->data = buffer.read(size);

        if (entry->loadImage()) {
            m_entries.push_back(entry);
            qDebug() << "Found valid image of size" << size;
        }
    }

    if (!m_entries.empty()) {
        qDebug() << "Format 2 succeeded with" << m_entries.size() << "entries";
        return true;
    }
    return false;
}

bool PakReader::tryFormat3(const QByteArray& data) {
    qDebug() << "Trying Format 3...";
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QBuffer::ReadOnly);

    // Try to find image data by scanning for common headers
    m_entries.clear();
    QByteArray chunk;
    int chunkSize = 4096;  // Read in 4KB chunks
    
    while (!buffer.atEnd()) {
        chunk = buffer.read(chunkSize);
        
        for (int i = 0; i < chunk.size(); i++) {
            // Look for PNG signature
            if (i + 8 <= chunk.size() && 
                chunk.mid(i, 8) == QByteArray::fromHex("89504E470D0A1A0A")) {
                qDebug() << "Found PNG signature at offset" << buffer.pos() - chunk.size() + i;
                
                // Try to extract the image
                auto entry = std::make_shared<SpriteEntry>();
                entry->name = QString("sprite_%1").arg(m_entries.size());
                
                // Read potential image data
                buffer.seek(buffer.pos() - chunk.size() + i);
                entry->data = buffer.read(32768);  // Read a reasonable amount
                
                if (entry->loadImage()) {
                    m_entries.push_back(entry);
                }
            }
        }
    }

    if (!m_entries.empty()) {
        qDebug() << "Format 3 succeeded with" << m_entries.size() << "entries";
        return true;
    }
    return false;
}

int PakReader::findPNGEnd(const QByteArray& data, int start) {
    QByteArray iendMarker = QByteArray::fromHex("49454E44AE426082");
    int endPos = data.indexOf(iendMarker, start);
    if (endPos != -1) {
        return endPos + iendMarker.size() - start;
    }
    return -1;
}

bool SpriteEntry::loadImage() {
    try {
        QBuffer buffer(&data);
        buffer.open(QBuffer::ReadOnly);
        
        // Try different image formats
        if (image.load(&buffer, "PNG")) {
            qDebug() << "Loaded as PNG, size:" << image.size();
            return true;
        }
        
        buffer.seek(0);
        if (image.load(&buffer, "BMP")) {
            qDebug() << "Loaded as BMP, size:" << image.size();
            return true;
        }
        
        buffer.seek(0);
        if (image.load(&buffer, "JPG")) {
            qDebug() << "Loaded as JPG, size:" << image.size();
            return true;
        }
        
        // Try different raw image formats
        for (int width = 16; width <= 256; width *= 2) {
            for (int height = 16; height <= 256; height *= 2) {
                QImage img(reinterpret_cast<const uchar*>(data.constData()), 
                         width, height, QImage::Format_ARGB32);
                if (!img.isNull() && !img.isGrayscale()) {
                    image = img;
                    qDebug() << "Loaded as raw" << width << "x" << height;
                    return true;
                }
            }
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