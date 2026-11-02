#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "gametypes.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class GameWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onStartClicked();
    void onSelectClicked();
    void onGameOver();

private:
    Ui::MainWindow* ui;
    GameWidget* gameWidget = nullptr;
    int selectedClassIndex = 0; // 0: archer, 1: warrior, 2: tanker

    void updateClassLabel();
    void startGame();
};
#endif // MAINWINDOW_H