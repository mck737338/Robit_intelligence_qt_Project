#include "entity.hpp"
#include <cmath>

// ============ entity ============
entity::entity() { pos.x = 0; pos.y = 0; }
entity::entity(int x, int y) { pos.x = x; pos.y = y; }

entity::Position entity::getPosition() { return pos; }
void entity::setPosition(int x, int y) { pos.x = x; pos.y = y; }
void entity::move(int dx, int dy) { pos.x += dx; pos.y += dy; }

float entity::getDistance(entity* object) {
    float dx = static_cast<float>(this->pos.x - object->pos.x);
    float dy = static_cast<float>(this->pos.y - object->pos.y);
    return std::sqrt(dx * dx + dy * dy);
}

// ============ isLife ============
void isLife::healed(int parameter) {
    life += parameter;
    if (life > max_life) life = max_life;
}

void isLife::hurted(float damage) {
    if (damage <= 0) return;
    double mitigation = 100.0 / (100.0 + defense);
    int actual_damage = static_cast<int>(damage * mitigation);
    if (actual_damage < 0) actual_damage = 0;
    life -= actual_damage;
    if (life < 0) life = 0;
}

int isLife::getLife() { return life; }
int isLife::getMaxLife() { return max_life; }
int isLife::getSpeed() { return speed; }
int isLife::getAttackSpeed() { return attack_speed; }
int isLife::getAttackRange() { return attack_range; }
int isLife::getPower() { return power; }
int isLife::getDefense() { return defense; }
bool isLife::isDead() { return life <= 0; }

// speed/attack_speed 값이 클수록 쿨다운이 짧아지도록 변환
static constexpr double MOVE_BASE_MS = 12000.0;
static constexpr double ATTACK_BASE_MS = 12000.0;

TimeMs isLife::getMoveCooldownMs() const {
    if (speed <= 0) return -1;
    return static_cast<TimeMs>(MOVE_BASE_MS / speed);
}
TimeMs isLife::getAttackCooldownMs() const {
    if (attack_speed <= 0) return -1;
    return static_cast<TimeMs>(ATTACK_BASE_MS / attack_speed);
}
bool isLife::canMove(TimeMs now) const {
    TimeMs cd = getMoveCooldownMs();
    if (cd < 0) return false;
    return (now - lastMoveTime) >= cd;
}
bool isLife::canAttack(TimeMs now) const {
    TimeMs cd = getAttackCooldownMs();
    if (cd < 0) return false;
    return (now - lastAttackTime) >= cd;
}
void isLife::markMove(TimeMs now) { lastMoveTime = now; }
void isLife::markAttack(TimeMs now) { lastAttackTime = now; }

// ============ player ============
player::player() : entity(), isLife() {}
player::player(int x, int y) : entity(x, y), isLife() {}

// archer: 체력 중간, 방어 중간, 힘 높음, 이동 높음, 공속 중상
archer::archer() : player() {
    max_life = 60; life = max_life;
    defense = 60; power = 100;
    speed = 90; attack_speed = 80;
    attack_range = 6;
}
archer::archer(int x, int y) : player(x, y) {
    max_life = 60; life = max_life;
    defense = 60; power = 100;
    speed = 90; attack_speed = 80;
    attack_range = 6;
}
void archer::attack(isLife* object) {
    entity* target = dynamic_cast<entity*>(object);
    if (!target) return;
    float distance = this->getDistance(target);
    if (distance <= attack_range) {
        int damage = static_cast<int>(0.7 * power * distance / attack_range);
        object->hurted(static_cast<float>(damage));
    }
}

// warrior: 체력 중상, 방어 중상, 힘 중간, 이동 중간, 공속 중간
warrior::warrior() : player() {
    max_life = 80; life = max_life;
    defense = 80; power = 60;
    speed = 60; attack_speed = 60;
    attack_range = 2;
}
warrior::warrior(int x, int y) : player(x, y) {
    max_life = 80; life = max_life;
    defense = 80; power = 60;
    speed = 60; attack_speed = 60;
    attack_range = 2;
}
void warrior::attack(isLife* object) {
    entity* target = dynamic_cast<entity*>(object);
    if (!target) return;
    if (this->getDistance(target) <= attack_range) {
        object->hurted(power * 0.8f);
    }
}

// tanker: 체력 높음, 방어 높음, 힘 중간, 이동 중하, 공속 중하 + 넉백
tanker::tanker() : player() {
    max_life = 100; life = max_life;
    defense = 100; power = 60;
    speed = 40; attack_speed = 40;
    attack_range = 4;
}
tanker::tanker(int x, int y) : player(x, y) {
    max_life = 100; life = max_life;
    defense = 100; power = 60;
    speed = 40; attack_speed = 40;
    attack_range = 4;
}
void tanker::attack(isLife* object) {
    entity* target = dynamic_cast<entity*>(object);
    if (!target) return;
    if (this->getDistance(target) <= attack_range) {
        object->hurted(power * 0.6f);
        entity::Position my_pos = this->getPosition();
        entity::Position target_pos = target->getPosition();
        if (target_pos.x >= my_pos.x) target->move(2, 0);
        else target->move(-2, 0);
    }
}

// ============ enemy ============
enemy::enemy() : entity(), isLife() {}
enemy::enemy(int x, int y) : entity(x, y), isLife() {}

// zombie: 체력 중하, 방어 중간, 힘 중하, 이동 중간, 공속 중간
zombie::zombie() : enemy() {
    max_life = 70; life = max_life;
    defense = 60; power = 40;
    speed = 60; attack_speed = 60;
    attack_range = 1;
}
zombie::zombie(int x, int y) : enemy(x, y) {
    max_life = 70; life = max_life;
    defense = 60; power = 40;
    speed = 60; attack_speed = 60;
    attack_range = 1;
}
void zombie::attack(isLife* object) {
    entity* target = dynamic_cast<entity*>(object);
    if (!target) return;
    if (this->getDistance(target) <= attack_range) {
        object->hurted(power * 0.7f);
    }
}

// boomber: 체력 낮음, 방어 낮음, 힘 높음, 이동 높음, 공속 매우 높음
boomber::boomber() : enemy() {
    max_life = 20; life = max_life;
    defense = 20; power = 100;
    speed = 110; attack_speed = 200;
    attack_range = 1;
}
boomber::boomber(int x, int y) : enemy(x, y) {
    max_life = 20; life = max_life;
    defense = 20; power = 100;
    speed = 110; attack_speed = 200;
    attack_range = 1;
}
void boomber::attack(isLife* object) {
    entity* target = dynamic_cast<entity*>(object);
    if (!target) return;
    if (this->getDistance(target) <= attack_range) {
        object->hurted(power * 0.7f);
    }
}