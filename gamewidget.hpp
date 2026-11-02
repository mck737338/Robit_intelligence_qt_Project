#ifndef GAMEWIDGET_HPP
#define GAMEWIDGET_HPP

#include <QWidget>
#include <QElapsedTimer>
#include <QTimer>
#include <QSet>
#include <QColor>
#include <vector>
#include "entity.hpp"
#include "map.hpp"
#include "gametypes.h"

// 공격/피격 시 튀는 파티클 하나
struct Particle {
    float x;
    float y;
    float vx;
    float vy;
    float life;      // 남은 수명(초)
    float maxLife;   // 초기 수명(초, 알파값 계산용)
    QColor color;
};

class GameWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameWidget(ClassType cls, QWidget* parent = nullptr);
    ~GameWidget() override;

signals:
    void gameOver();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void gameTick();
    void spawnTick();

private:
    map* gameMap = nullptr;
    int loadAccumX = 0; // 마지막 load() 이후 플레이어가 x축으로 누적 이동한 거리
    player* thePlayer = nullptr;
    std::vector<enemy*> enemies;
    std::vector<Particle> particles;

    QTimer tickTimer;
    QTimer spawnTimer;
    QElapsedTimer clock;
    qint64 lastFrameMs = 0;

    QSet<int> heldKeys;
    bool gameEnded = false;

    template <typename EntLife>
    bool tryStepMove(entity* ent, EntLife* life, int dx, TimeMs now);

    void handlePlayerMovement(TimeMs now);
    void handleEnemyMovement(TimeMs now);
    void handleAttacks(TimeMs now);
    void checkMapEdgeAndScroll();
    void shiftAllEntities(int delta);
    void cleanupDeadEnemies();

    // 파티클 관련
    void spawnHitParticles(float worldX, float worldY, const QColor& color, int count = 14);
    void updateParticles(float dtSeconds);
};

#endif // GAMEWIDGET_HPP