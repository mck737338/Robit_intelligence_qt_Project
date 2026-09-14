#include "gamewidget.hpp"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

namespace {
// ---- 속도/해상도 튜닝 상수 ----
constexpr int   TICK_MS            = 16;   // 30 -> 16 (약 60fps로 더 촘촘하게 갱신, 조작 반응성 상승)
constexpr int   SPAWN_INTERVAL_MS  = 3000;
constexpr int   SPAWN_DISTANCE     = 10;
constexpr int   MAP_WIDTH          = 41;
constexpr int   MAP_MAX_HEIGHT     = 5;
constexpr int   PIXELS_PER_CELL    = 48;   // 20 -> 48, 맵 1칸 크기 확대

// 이동/공격 쿨다운 기준값을 낮춰 전체 조작 속도를 상승시킴
// (isLife::getMoveCooldownMs / getAttackCooldownMs 가 MOVE_BASE_MS / stat 형태이므로,
//  entity.cpp의 MOVE_BASE_MS, ATTACK_BASE_MS 상수를 함께 낮춰야 실제 반영됩니다. 하단 3번 참고)
}

GameWidget::GameWidget(ClassType cls, QWidget* parent) : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(false);
    setMinimumSize(1280, 720); // 해상도 상승: 게임 화면 최소 크기 확대

    gameMap = new map(MAP_WIDTH, MAP_MAX_HEIGHT);

    switch (cls) {
    case ClassType::Archer:  thePlayer = new archer(0, 0);  break;
    case ClassType::Warrior: thePlayer = new warrior(0, 0); break;
    case ClassType::Tanker:  thePlayer = new tanker(0, 0);  break;
    }
    thePlayer->setPosition(0, gameMap->isWall(0));

    clock.start();
    lastFrameMs = clock.elapsed();

    connect(&tickTimer, &QTimer::timeout, this, &GameWidget::gameTick);
    connect(&spawnTimer, &QTimer::timeout, this, &GameWidget::spawnTick);
    tickTimer.start(TICK_MS);
    spawnTimer.start(SPAWN_INTERVAL_MS);
}

GameWidget::~GameWidget() {
    delete thePlayer;
    for (auto* e : enemies) delete e;
    delete gameMap;
}

// ---------------- 입력 ----------------
void GameWidget::keyPressEvent(QKeyEvent* event) {
    heldKeys.insert(event->key());
}
void GameWidget::keyReleaseEvent(QKeyEvent* event) {
    heldKeys.remove(event->key());
}

void GameWidget::mousePressEvent(QMouseEvent* /*event*/) {
    TimeMs now = clock.elapsed();
    if (gameEnded) return;
    if (thePlayer->canAttack(now)) {
        entity::Position pp = thePlayer->getPosition();
        bool hitAny = false;
        for (auto* e : enemies) {
            if (!e->isDead()) {
                int lifeBefore = e->getLife();
                thePlayer->attack(e);
                if (e->getLife() < lifeBefore) {
                    hitAny = true;
                    entity::Position ep = e->getPosition();
                    // 적 위치에서 노란색 계열 타격 파티클
                    spawnHitParticles(static_cast<float>(ep.x), static_cast<float>(ep.y),
                                       QColor(255, 210, 80));
                }
            }
        }
        if (!hitAny) {
            // 허공을 휘두른 경우에도 플레이어 앞쪽에 약한 파티클(피드백용)
            spawnHitParticles(static_cast<float>(pp.x), static_cast<float>(pp.y),
                               QColor(200, 200, 220), 6);
        }
        thePlayer->markAttack(now);
    }
}

// ---------------- 이동/등반 공통 처리 ----------------
template <typename EntLife>
bool GameWidget::tryStepMove(entity* ent, EntLife* life, int dx, TimeMs now) {
    if (!life->canMove(now)) return false;

    entity::Position pos = ent->getPosition();
    int aheadX = pos.x + dx;
    int aheadHeight = gameMap->isWall(aheadX);

    if (aheadHeight > pos.y) {
        ent->move(0, 1);
    } else if (gameMap->isWall(pos.x) == pos.y || aheadHeight == pos.y) {
        ent->move(dx, 0);
    } else {
        ent->move(0, -1);
    }

    life->markMove(now);
    return true;
}

// ---------------- 플레이어 이동 ----------------
void GameWidget::handlePlayerMovement(TimeMs now) {
    bool left = heldKeys.contains(Qt::Key_A) || heldKeys.contains(Qt::Key_Left);
    bool right = heldKeys.contains(Qt::Key_D) || heldKeys.contains(Qt::Key_Right);
    if (left == right) return;

    int dx = right ? 1 : -1;

    entity::Position before = thePlayer->getPosition();
    bool moved = tryStepMove(thePlayer, thePlayer, dx, now);
    entity::Position after = thePlayer->getPosition();

    if (moved) {
        // 등반/하강(y만 변함)일 때는 0, 실제 x 이동일 때만 누적됨
        loadAccumX += (after.x - before.x);
    }

    checkMapEdgeAndScroll();
}

// ---------------- 적 이동 ----------------
void GameWidget::handleEnemyMovement(TimeMs now) {
    entity::Position pp = thePlayer->getPosition();
    for (auto* e : enemies) {
        if (e->isDead()) continue;
        entity::Position ep = e->getPosition();
        int dx = 0;
        if (ep.x < pp.x) dx = 1;
        else if (ep.x > pp.x) dx = -1;
        if (dx != 0) tryStepMove(e, e, dx, now);
    }
}

// ---------------- 공격 처리 (적 -> 플레이어) ----------------
void GameWidget::handleAttacks(TimeMs now) {
    for (auto* e : enemies) {
        if (e->isDead()) continue;
        float dist = e->getDistance(thePlayer);
        if (dist <= e->getAttackRange() && e->canAttack(now)) {
            int lifeBefore = thePlayer->getLife();
            e->attack(thePlayer);
            if (thePlayer->getLife() < lifeBefore) {
                entity::Position pp = thePlayer->getPosition();
                // 플레이어가 맞았을 때 붉은색 계열 파티클
                spawnHitParticles(static_cast<float>(pp.x), static_cast<float>(pp.y),
                                   QColor(255, 90, 90));
            }
            e->markAttack(now);
        }
    }
}

// ---------------- 맵 스크롤 & 좌표 보정 ----------------
void GameWidget::checkMapEdgeAndScroll() {
    // 오른쪽으로 10칸 누적 이동 시 load(1) 실행
    while (loadAccumX >= 10) {
        int loaded = gameMap->load(1); // RIGHT 로드
        if (loaded > 0) {
            shiftAllEntities(-loaded); // 로드 방향과 반대로 좌표 보정
        }
        loadAccumX -= 10;
    }

    // 왼쪽으로 10칸 누적 이동 시 load(0) 실행
    while (loadAccumX <= -10) {
        int loaded = gameMap->load(0); // LEFT 로드
        if (loaded > 0) {
            shiftAllEntities(loaded); // 로드 방향과 반대로 좌표 보정
        }
        loadAccumX += 10;
    }
}

void GameWidget::shiftAllEntities(int delta) {
    auto shift = [this, delta](entity* ent) {
        entity::Position pos = ent->getPosition();
        int newX = pos.x + delta;
        ent->setPosition(newX, gameMap->isWall(newX));
    };
    shift(thePlayer);
    for (auto* e : enemies) shift(e);

    // 맵이 스크롤되면 화면에 남아있는 파티클도 같이 밀어줘야 어긋나지 않음
    for (auto& p : particles) {
        p.x += delta;
    }
}

void GameWidget::cleanupDeadEnemies() {
    for (auto it = enemies.begin(); it != enemies.end();) {
        if ((*it)->isDead()) {
            entity::Position ep = (*it)->getPosition();
            // 처치 시 큼직한 파티클 폭발
            spawnHitParticles(static_cast<float>(ep.x), static_cast<float>(ep.y),
                               QColor(255, 150, 60), 24);
            delete *it;
            it = enemies.erase(it);
        } else {
            ++it;
        }
    }
}

// ---------------- 파티클 ----------------
void GameWidget::spawnHitParticles(float worldX, float worldY, const QColor& color, int count) {
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = worldX;
        p.y = worldY + 0.5f; // 대략 몸통 중앙 높이에서 발생
        float angle = rng->bounded(360) * static_cast<float>(M_PI) / 180.0f;
        float speed = 1.5f + rng->bounded(100) / 100.0f * 2.5f; // world-units/sec
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.maxLife = 0.35f + rng->bounded(100) / 100.0f * 0.25f; // 0.35~0.6초
        p.life = p.maxLife;
        p.color = color;
        particles.push_back(p);
    }
}

void GameWidget::updateParticles(float dtSeconds) {
    for (auto& p : particles) {
        p.x += p.vx * dtSeconds;
        p.y += p.vy * dtSeconds;
        p.vy -= 4.0f * dtSeconds; // 약한 중력(위로 튀었다가 떨어지는 느낌)
        p.life -= dtSeconds;
    }
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
                        [](const Particle& p) { return p.life <= 0.0f; }),
        particles.end());
}

// ---------------- 스폰 ----------------
void GameWidget::spawnTick() {
    if (gameEnded) return;
    entity::Position pp = thePlayer->getPosition();
    bool rightSide = QRandomGenerator::global()->bounded(2) == 0;
    int spawnX = rightSide ? pp.x + SPAWN_DISTANCE : pp.x - SPAWN_DISTANCE;
    int spawnY = gameMap->isWall(spawnX);

    enemy* e = (QRandomGenerator::global()->bounded(2) == 0)
                   ? static_cast<enemy*>(new zombie(spawnX, spawnY))
                   : static_cast<enemy*>(new boomber(spawnX, spawnY));
    enemies.push_back(e);
}

// ---------------- 메인 루프 ----------------
void GameWidget::gameTick() {
    if (gameEnded) return;
    TimeMs now = clock.elapsed();
    float dtSeconds = static_cast<float>(now - lastFrameMs) / 1000.0f;
    lastFrameMs = now;

    handlePlayerMovement(now);
    handleEnemyMovement(now);
    handleAttacks(now);
    cleanupDeadEnemies();
    updateParticles(dtSeconds);

    if (thePlayer->isDead()) {
        gameEnded = true;
        tickTimer.stop();
        spawnTimer.stop();
        emit gameOver();
    }

    update();
}

// ---------------- 렌더링 ----------------
void GameWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(30, 30, 40));

    int centerX = width() / 2;
    int groundY = height() - 100;

    entity::Position pp = thePlayer->getPosition();

    auto toScreenX = [&](float worldX) {
        return centerX + (worldX - pp.x) * PIXELS_PER_CELL;
    };
    auto toScreenY = [&](float worldY) {
        return groundY - worldY * PIXELS_PER_CELL;
    };

    // 지형
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(80, 80, 90));
    int visibleHalf = width() / (2 * PIXELS_PER_CELL) + 2;
    for (int wx = pp.x - visibleHalf; wx <= pp.x + visibleHalf; ++wx) {
        int h = gameMap->isWall(wx);
        int sx = static_cast<int>(toScreenX(wx));
        int sy = static_cast<int>(toScreenY(h));
        painter.drawRect(sx, sy, PIXELS_PER_CELL, height() - sy);
    }

    // 플레이어
    painter.setBrush(QColor(80, 180, 255));
    int psx = static_cast<int>(toScreenX(pp.x));
    int psy = static_cast<int>(toScreenY(pp.y)) - PIXELS_PER_CELL;
    painter.drawRect(psx, psy, PIXELS_PER_CELL, PIXELS_PER_CELL);

    // 적
    painter.setBrush(QColor(220, 80, 80));
    for (auto* e : enemies) {
        if (e->isDead()) continue;
        entity::Position ep = e->getPosition();
        int esx = static_cast<int>(toScreenX(ep.x));
        int esy = static_cast<int>(toScreenY(ep.y)) - PIXELS_PER_CELL;
        painter.drawRect(esx, esy, PIXELS_PER_CELL, PIXELS_PER_CELL);
    }

    // 파티클 (수명에 따라 페이드아웃 + 크기 축소)
    for (const auto& p : particles) {
        float t = p.life / p.maxLife; // 1.0(생성 직후) -> 0.0(소멸 직전)
        QColor c = p.color;
        c.setAlphaF(std::clamp(t, 0.0f, 1.0f));
        painter.setBrush(c);
        float radius = 3.0f + 4.0f * t; // 초기엔 크고 점점 작아짐
        float sx = toScreenX(p.x);
        float sy = toScreenY(p.y);
        painter.drawEllipse(QPointF(sx, sy), radius, radius);
    }

    // 체력바
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(14); // 해상도 상승에 맞춰 폰트도 확대
    painter.setFont(font);
    painter.drawText(16, 30, QString("HP: %1 / %2")
                                  .arg(thePlayer->getLife())
                                  .arg(thePlayer->getMaxLife()));
}