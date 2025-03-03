#include "pakreader.h"
#include <QFile>
#include <QDataStream>
#include <QBuffer>
#include <QDebug>

PakReader::PakReader() {}

bool PakReader::readFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file:" << file.errorString();
        return false;
    }

    // Clear existing entries
    m_entries.clear();

    // Read and verify header (16 bytes total)
    QByteArray headerData = file.read(16);
    qDebug() << "Full header:" << headerData.toHex();
    
    // Skip the header text "<Pak file here>" (16 bytes)
    if (!headerData.startsWith("<Pak file here>")) {
        qDebug() << "Invalid header format";
        return false;
    }

    // Read the number of entries (4 bytes)
    quint32 numEntries;
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream >> numEntries;
    
    qDebug() << "Number of entries:" << numEntries;

    // Read file table
    for (quint32 i = 0; i < numEntries && !stream.atEnd(); i++) {
        auto entry = std::make_shared<SpriteEntry>();

        // Read name (null-terminated string)
        QByteArray nameBuf;
        char c;
        while (stream.readRawData(&c, 1) == 1 && c != '\0') {
            nameBuf.append(c);
        }
        entry->name = QString::fromUtf8(nameBuf);

        // Read data size (4 bytes)
        quint32 size;
        stream >> size;

        // Read the actual sprite data
        entry->data = file.read(size);

        if (entry->data.size() != size) {
            qDebug() << "Failed to read complete data for entry" << i;
            return false;
        }

        qDebug() << "Entry" << i << ":" << entry->name << "Size:" << size;
        m_entries.push_back(entry);
    }

    return !m_entries.empty();
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
    QBuffer buffer(&data);
    buffer.open(QBuffer::ReadOnly);

    // Try to load as standard image formats first
    if (image.load(&buffer, "PNG") || 
        image.load(&buffer, "BMP") || 
        image.load(&buffer, "JPG")) {
        return true;
    }

    // If standard formats fail, try to load as raw image data
    // The game might use a custom format, so we'll try common dimensions
    buffer.seek(0);
    
    // Try to detect image dimensions from the data size
    int dataSize = data.size();
    
    // Common sprite sizes to try (32x32, 64x64, 128x128, etc)
    const int sizes[] = {32, 64, 128, 256, 512};
    
    for (int width : sizes) {
        for (int height : sizes) {
            // Try 32-bit RGBA
            if (dataSize == width * height * 4) {
                image = QImage((const uchar*)data.constData(), width, height, 
                             width * 4, QImage::Format_RGBA8888);
                if (!image.isNull()) return true;
            }
            // Try 24-bit RGB
            if (dataSize == width * height * 3) {
                image = QImage((const uchar*)data.constData(), width, height, 
                             width * 3, QImage::Format_RGB888);
                if (!image.isNull()) return true;
            }
        }
    }

    qDebug() << "Failed to load image data of size" << dataSize;
    return false;
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