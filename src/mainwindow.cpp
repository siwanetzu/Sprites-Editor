#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QToolBar>
#include <QIcon>
#include <QApplication>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Sprite Editor");
    resize(1200, 800);
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupConnections();
}

void MainWindow::setupUI()
{
    auto centralWidget = new QWidget(this);
    auto mainLayout = new QHBoxLayout(centralWidget);
    
    auto splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter);

    // Left panel - Sprite Tree
    spriteTree = new QTreeWidget;
    spriteTree->setHeaderLabel("Sprites");
    spriteTree->setMinimumWidth(250);
    splitter->addWidget(spriteTree);

    // Right panel
    auto rightPanel = new QWidget;
    auto rightLayout = new QVBoxLayout(rightPanel);

    // Preview area with zoom controls
    auto previewContainer = new QWidget;
    auto previewLayout = new QVBoxLayout(previewContainer);

    previewScroll = new QScrollArea;
    previewScroll->setWidgetResizable(true);
    previewLabel = new QLabel;
    previewLabel->setAlignment(Qt::AlignCenter);
    previewScroll->setWidget(previewLabel);
    previewLayout->addWidget(previewScroll);

    // Zoom controls
    auto zoomLayout = new QHBoxLayout;
    auto zoomInBtn = new QPushButton("+");
    auto zoomOutBtn = new QPushButton("-");
    auto resetZoomBtn = new QPushButton("Reset Zoom");
    zoomLayout->addWidget(zoomOutBtn);
    zoomLayout->addWidget(resetZoomBtn);
    zoomLayout->addWidget(zoomInBtn);
    previewLayout->addLayout(zoomLayout);

    rightLayout->addWidget(previewContainer);

    // Export controls
    auto exportLayout = new QHBoxLayout;
    exportPngButton = new QPushButton("Export as PNG");
    exportBmpButton = new QPushButton("Export as BMP");
    exportLayout->addWidget(exportPngButton);
    exportLayout->addWidget(exportBmpButton);
    rightLayout->addLayout(exportLayout);

    splitter->addWidget(rightPanel);
    setCentralWidget(centralWidget);
}

void MainWindow::setupMenuBar()
{
    auto menuBar = new QMenuBar(this);
    auto fileMenu = menuBar->addMenu("&File");
    
    auto openAction = fileMenu->addAction("&Open PAK File...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::loadPakFile);
    
    fileMenu->addSeparator();
    
    auto exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);
    
    setMenuBar(menuBar);
}

void MainWindow::setupToolBar()
{
    auto toolBar = addToolBar("Main Toolbar");
    toolBar->setMovable(false);
    
    auto openAction = toolBar->addAction("Open PAK");
    connect(openAction, &QAction::triggered, this, &MainWindow::loadPakFile);
    
    toolBar->addSeparator();
    
    auto zoomInAction = toolBar->addAction("Zoom In");
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    
    auto zoomOutAction = toolBar->addAction("Zoom Out");
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
}

void MainWindow::setupStatusBar()
{
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");
}

void MainWindow::loadPakFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Open PAK File", "", "PAK Files (*.pak);;All Files (*)");
        
    if (fileName.isEmpty())
        return;
        
    statusBar->showMessage("Loading PAK file...");
    QApplication::processEvents(); // Allow UI to update
        
    if (!currentPak.readFile(fileName)) {
        QMessageBox::critical(this, "Error", "Failed to open PAK file. Check debug output for details.");
        statusBar->showMessage("Failed to load PAK file");
        return;
    }
    
    currentFile = fileName;
    spriteTree->clear();
    
    // Create root item for the PAK file
    auto rootItem = new QTreeWidgetItem(spriteTree);
    rootItem->setText(0, QFileInfo(fileName).fileName());
    
    // Add all sprites as children
    int validSprites = 0;
    for (const auto& entry : currentPak.entries()) {
        if (entry->image.isNull()) {
            qDebug() << "Skipping invalid sprite:" << entry->name;
            continue;
        }
        auto item = new QTreeWidgetItem(rootItem);
        item->setText(0, entry->name);
        item->setData(0, Qt::UserRole, QVariant::fromValue(entry));
        validSprites++;
    }
    
    rootItem->setExpanded(true);
    statusBar->showMessage(QString("Loaded %1 valid sprites").arg(validSprites));
}

void MainWindow::displaySprite(const QModelIndex& index)
{
    auto item = spriteTree->currentItem();
    if (!item)
        return;
        
    auto sprite = item->data(0, Qt::UserRole).value<std::shared_ptr<SpriteEntry>>();
    if (!sprite)
        return;
        
    currentSprite = sprite;
    QImage preview = sprite->preview();
    
    if (preview.isNull()) {
        previewLabel->setText("Preview not available");
        return;
    }
    
    // Apply zoom
    if (zoomLevel != 1.0) {
        preview = preview.scaled(preview.size() * zoomLevel, 
                               Qt::KeepAspectRatio, 
                               Qt::SmoothTransformation);
    }
    
    previewLabel->setPixmap(QPixmap::fromImage(preview));
    statusBar->showMessage(QString("Sprite size: %1x%2")
        .arg(preview.width())
        .arg(preview.height()));
}

void MainWindow::zoomIn()
{
    zoomLevel *= ZOOM_FACTOR;
    displaySprite(QModelIndex()); // Refresh display
}

void MainWindow::zoomOut()
{
    zoomLevel /= ZOOM_FACTOR;
    displaySprite(QModelIndex()); // Refresh display
}

void MainWindow::resetZoom()
{
    zoomLevel = 1.0;
    displaySprite(QModelIndex()); // Refresh display
}

void MainWindow::setupConnections()
{
    connect(spriteTree, &QTreeWidget::itemClicked, 
            [this](QTreeWidgetItem* item, int) {
                displaySprite(QModelIndex());
            });
            
    connect(exportPngButton, &QPushButton::clicked, 
            [this]() { exportSprite("PNG"); });
            
    connect(exportBmpButton, &QPushButton::clicked, 
            [this]() { exportSprite("BMP"); });
}

void MainWindow::exportSprite(const QString& format)
{
    if (!currentSprite)
        return;
        
    QString fileName = QFileDialog::getSaveFileName(this,
        QString("Export as %1").arg(format),
        QString("%1.%2").arg(currentSprite->name, format.toLower()),
        QString("%1 Files (*.%2)").arg(format, format.toLower()));
        
    if (fileName.isEmpty())
        return;
        
    if (!currentSprite->exportTo(fileName, format)) {
        QMessageBox::warning(this, "Export Failed", 
            "Failed to export sprite");
    }
} 