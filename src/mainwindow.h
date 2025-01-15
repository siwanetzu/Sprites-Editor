#pragma once

#include <QMainWindow>
#include <QListView>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QTreeWidget>
#include <QStatusBar>
#include "spritemodel.h"
#include "pakreader.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void loadPakFile();
    void displaySprite(const QModelIndex& index);
    void exportSprite(const QString& format);
    void zoomIn();
    void zoomOut();
    void resetZoom();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupConnections();

    // UI Components
    QTreeWidget* spriteTree;
    QLabel* previewLabel;
    QScrollArea* previewScroll;
    QPushButton* exportPngButton;
    QPushButton* exportBmpButton;
    QStatusBar* statusBar;
    
    // Zoom controls
    float zoomLevel = 1.0;
    static constexpr float ZOOM_FACTOR = 1.2f;
    
    PakReader currentPak;
    QString currentFile;
    std::shared_ptr<SpriteEntry> currentSprite;
}; 