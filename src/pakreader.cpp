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

    // Debug file size
    qDebug() << "File size:" << file.size() << "bytes";

    try {
        // Read and verify header
        QByteArray header = file.read(4);
        qDebug() << "Header:" << header;

        // If not a standard PAK file, try alternative format
        if (header != "PACK") {
            qDebug() << "Not a standard PAK file, trying alternative format";
            file.seek(0);
        }

        // Read file size and number of entries with bounds checking
        if (file.bytesAvailable() < 8) {
            qDebug() << "File too small for header";
            return false;
        }

        quint32 fileSize, numEntries;
        stream >> fileSize >> numEntries;

        // Sanity check
        if (numEntries > 10000) { // Arbitrary reasonable limit
            qDebug() << "Too many entries:" << numEntries;
            return false;
        }

        qDebug() << "File size from header:" << fileSize;
        qDebug() << "Number of entries:" << numEntries;

        m_entries.clear();
        m_entries.reserve(numEntries);

        // Read entries
        for (quint32 i = 0; i < numEntries; ++i) {
            if (file.bytesAvailable() < 9) { // Minimum bytes needed for an entry
                qDebug() << "Unexpected end of file at entry" << i;
                return false;
            }

            quint32 offset, size;
            quint8 nameLength;
            stream >> offset >> size >> nameLength;

            // Sanity checks
            if (nameLength > 100) { // Arbitrary reasonable limit
                qDebug() << "Name length too large:" << nameLength;
                return false;
            }

            if (file.bytesAvailable() < nameLength) {
                qDebug() << "Unexpected end of file reading name";
                return false;
            }

            QByteArray nameData = file.read(nameLength);
            QString name = QString::fromUtf8(nameData);

            // More sanity checks
            if (offset + size > file.size()) {
                qDebug() << "Entry" << name << "extends beyond file size";
                return false;
            }

            qDebug() << "Reading entry:" << name << "offset:" << offset << "size:" << size;

            auto entry = std::make_shared<SpriteEntry>();
            entry->name = name;

            // Store current position
            qint64 currentPos = file.pos();
            
            // Read sprite data
            file.seek(offset);
            entry->data = file.read(size);
            
            if (entry->data.size() != size) {
                qDebug() << "Failed to read complete data for" << name;
                return false;
            }

            // Try to load as image
            if (entry->loadImage()) {
                qDebug() << "Successfully loaded image for" << name;
            } else {
                qDebug() << "Failed to load image for" << name;
            }
            
            // Restore position for next entry
            file.seek(currentPos);
            
            m_entries.push_back(entry);
        }

        qDebug() << "Successfully loaded" << m_entries.size() << "entries";
        return true;

    } catch (const std::exception& e) {
        qDebug() << "Exception while reading PAK file:" << e.what();
        return false;
    } catch (...) {
        qDebug() << "Unknown exception while reading PAK file";
        return false;
    }
}

std::vector<std::shared_ptr<SpriteEntry>> PakReader::entries() const {
    return m_entries;
}

bool SpriteEntry::loadImage() {
    try {
        QBuffer buffer(&data);
        buffer.open(QBuffer::ReadOnly);
        
        // Try different image formats
        if (image.load(&buffer, "PNG")) {
            return true;
        }
        buffer.seek(0);
        if (image.load(&buffer, "BMP")) {
            return true;
        }
        buffer.seek(0);
        if (image.load(&buffer, "JPG")) {
            return true;
        }
        
        // Add more formats if needed
        return false;
    } catch (...) {
        qDebug() << "Exception while loading image";
        return false;
    }
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