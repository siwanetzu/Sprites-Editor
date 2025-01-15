#pragma once

#include <QAbstractListModel>
#include <vector>
#include <memory>
#include "pakreader.h"

class SpriteModel : public QAbstractListModel {
    Q_OBJECT

public:
    explicit SpriteModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    bool loadFolder(const QString& path);
    void setFilter(const QString& category);
    std::shared_ptr<SpriteEntry> spriteAt(const QModelIndex& index) const;

private:
    QString categorizeFile(const QString& filename) const;
    void updateFilteredSprites();

    struct SpriteFile {
        QString path;
        QString category;
        PakReader reader;
    };

    std::vector<SpriteFile> m_spriteFiles;
    std::vector<std::pair<int, std::shared_ptr<SpriteEntry>>> m_filteredSprites;
    QString m_currentFilter;
}; 