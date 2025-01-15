#include "spritemodel.h"
#include <QDir>
#include <QDirIterator>

SpriteModel::SpriteModel(QObject* parent)
    : QAbstractListModel(parent)
{}

int SpriteModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_filteredSprites.size());
}

QVariant SpriteModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

    if (role == Qt::DisplayRole) {
        const auto& sprite = m_filteredSprites[index.row()];
        return sprite.second->name;
    }

    return QVariant();
}

bool SpriteModel::loadFolder(const QString& path) {
    beginResetModel();
    m_spriteFiles.clear();
    m_filteredSprites.clear();

    QDirIterator it(path, {"*.pak"}, QDir::Files);
    while (it.hasNext()) {
        QString filePath = it.next();
        SpriteFile spriteFile;
        spriteFile.path = filePath;
        spriteFile.category = categorizeFile(QFileInfo(filePath).fileName());
        
        if (spriteFile.reader.readFile(filePath)) {
            m_spriteFiles.push_back(std::move(spriteFile));
        }
    }

    updateFilteredSprites();
    endResetModel();
    return true;
}

void SpriteModel::setFilter(const QString& category) {
    beginResetModel();
    m_currentFilter = category;
    updateFilteredSprites();
    endResetModel();
}

std::shared_ptr<SpriteEntry> SpriteModel::spriteAt(const QModelIndex& index) const {
    if (!index.isValid() || index.row() >= m_filteredSprites.size()) {
        return nullptr;
    }
    return m_filteredSprites[index.row()].second;
}

QString SpriteModel::categorizeFile(const QString& filename) const {
    QString lower = filename.toLower();
    
    if (lower.contains("character") || lower.contains("char") || 
        lower.contains("npc") || lower.contains("monster") || 
        lower.contains("mob")) {
        return "Characters";
    }
    if (lower.contains("item") || lower.contains("weapon") || 
        lower.contains("armor")) {
        return "Items";
    }
    if (lower.contains("effect") || lower.contains("spell") || 
        lower.contains("magic")) {
        return "Effects";
    }
    if (lower.contains("map") || lower.contains("tile") || 
        lower.contains("terrain")) {
        return "Maps";
    }
    if (lower.contains("interface") || lower.contains("ui") || 
        lower.contains("hud")) {
        return "Interface";
    }
    return "All";
}

void SpriteModel::updateFilteredSprites() {
    m_filteredSprites.clear();
    
    for (size_t fileIndex = 0; fileIndex < m_spriteFiles.size(); ++fileIndex) {
        const auto& file = m_spriteFiles[fileIndex];
        if (m_currentFilter == "All" || file.category == m_currentFilter) {
            const auto& entries = file.reader.entries();
            for (const auto& entry : entries) {
                m_filteredSprites.emplace_back(fileIndex, entry);
            }
        }
    }
} 