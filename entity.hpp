#ifndef ENTITY_HPP
#define ENTITY_HPP

using TimeMs = long long;

class entity {
public:
    struct Position {
        int x;
        int y;
    };

    entity();
    entity(int x, int y);

    Position getPosition();
    void setPosition(int x, int y);
    void move(int dx, int dy);
    float getDistance(entity* object);

protected:
    Position pos;
};

class isLife {
protected:
    int life = 0;
    int max_life = 0;
    int defense = 0;
    int power = 0;
    int speed = 0;
    int attack_speed = 0;
    int attack_range = 0;

    TimeMs lastAttackTime = -1000000; // 초기값: 즉시 행동 가능하도록 매우 이전 시각
    TimeMs lastMoveTime = -1000000;

public:
    isLife() = default;
    virtual ~isLife() = default;

    void healed(int parameter);
    void hurted(float damage);

    int getLife();
    int getMaxLife();
    int getSpeed();
    int getAttackSpeed();
    int getAttackRange();
    int getPower();
    int getDefense();
    bool isDead();

    // 쿨다운 관련 (speed/attack_speed가 클수록 빠름)
    TimeMs getMoveCooldownMs() const;
    TimeMs getAttackCooldownMs() const;
    bool canMove(TimeMs now) const;
    bool canAttack(TimeMs now) const;
    void markMove(TimeMs now);
    void markAttack(TimeMs now);

    virtual void attack(isLife* object) = 0;
};

// ---------------- 플레이어 계열 ----------------
class player : public entity, public isLife {
public:
    player();
    player(int x, int y);
    virtual ~player() = default;
};

class archer : public player {
public:
    archer();
    archer(int x, int y);
    void attack(isLife* object) override;
};

class warrior : public player {
public:
    warrior();
    warrior(int x, int y);
    void attack(isLife* object) override;
};

class tanker : public player {
public:
    tanker();
    tanker(int x, int y);
    void attack(isLife* object) override;
};

// ---------------- 적 계열 ----------------
class enemy : public entity, public isLife {
public:
    enemy();
    enemy(int x, int y);
    virtual ~enemy() = default;
};

class zombie : public enemy {
public:
    zombie();
    zombie(int x, int y);
    void attack(isLife* object) override;
};

class boomber : public enemy {
public:
    boomber();
    boomber(int x, int y);
    void attack(isLife* object) override;
};

#endif // ENTITY_HPP