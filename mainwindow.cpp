#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "gamewidget.hpp"
#include <QKeyEvent>
#include <QVBoxLayout>

namespace {
const char* CLASS_NAMES[3] = { "궁수 (Archer)", "전사 (Warrior)", "탱커 (Tanker)" };
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(ui->btnSelectClass, &QPushButton::clicked, this, &MainWindow::onSelectClicked);
    connect(ui->btnPrevClass, &QPushButton::clicked, this, [this]() {
        selectedClassIndex = (selectedClassIndex + 2) % 3;
        updateClassLabel();
    });
    connect(ui->btnNextClass, &QPushButton::clicked, this, [this]() {
        selectedClassIndex = (selectedClassIndex + 1) % 3;
        updateClassLabel();
    });

    updateClassLabel();
    ui->stackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::updateClassLabel() {
    ui->labelClassName->setText(CLASS_NAMES[selectedClassIndex]);
}

void MainWindow::onStartClicked() {
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::onSelectClicked() {
    startGame();
    ui->stackedWidget->setCurrentIndex(2);
    if (gameWidget) gameWidget->setFocus();
}

void MainWindow::startGame() {
    if (gameWidget) {
        gameWidget->deleteLater();
        gameWidget = nullptr;
    }

    ClassType cls = static_cast<ClassType>(selectedClassIndex);
    gameWidget = new GameWidget(cls, ui->gameContainer);

    // gameContainer에 레이아웃이 없다면 새로 만들어 붙임
    if (!ui->gameContainer->layout()) {
        auto* layout = new QVBoxLayout(ui->gameContainer);
        layout->setContentsMargins(0, 0, 0, 0);
        ui->gameContainer->setLayout(layout);
    }
    ui->gameContainer->layout()->addWidget(gameWidget);

    connect(gameWidget, &GameWidget::gameOver, this, &MainWindow::onGameOver);
}

void MainWindow::onGameOver() {
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    // 직업 선택 화면에서 A/D, 좌우 화살표로도 직업 변경 가능
    if (ui->stackedWidget->currentIndex() == 1) {
        if (event->key() == Qt::Key_A || event->key() == Qt::Key_Left) {
            selectedClassIndex = (selectedClassIndex + 2) % 3;
            updateClassLabel();
            return;
        }
        if (event->key() == Qt::Key_D || event->key() == Qt::Key_Right) {
            selectedClassIndex = (selectedClassIndex + 1) % 3;
            updateClassLabel();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}