#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include "map.hpp"
using namespace std;

typedef struct recFormat {
    int obstacle;
    int item;
}recFormat;


map::map(int width, int height, int field_size, int frequency) {
    if (field_size <= 0) {
        this->field_size = width * 3;
    }
    else {
        this->field_size = field_size;
    }

    if(frequency <= 0){
        this->load_unit = width/3;
    }
    else {
        this->load_unit = frequency;
    }

    this->data.size = width;
    max_h = height;
    if (load_unit > this->data.size) {
        load_unit = this->data.size;
    }

    reset();
}

void map::reset() {
    srand((unsigned int)time(NULL));
    this->data.bndL = new Node;
    Node* ptr = this->data.bndL;

    for (int i = 0; i < this->data.size; i++) {
        // 현재 노드에 장애물 높이와 아이템 정보 채우기
        
        ptr->obstacle = max_h - 1 - (int)sqrt(rand() % max_h*max_h + 1);      // 0 ~ max_h 사이 무작위 높이
        ptr->item = (rand() % 10 == 0) ? 1 : 0;    // 예: 10% 확률로 아이템 배치

        if (i == this->data.size - 1) {
            // 마지막 노드 처리
            ptr->right = nullptr;
            this->data.bndR = ptr;
        }
        else {
            // 다음 노드를 새로 만들고 이중 연결
            Node* next = new Node;
            next->left = ptr;
            ptr->right = next;
            ptr = next;
        }
    }

    // 맨 왼쪽 노드의 left는 항상 nullptr
    this->data.bndL->left = nullptr;

    // ---------- 2) 로드된 맵 바깥 영역을 파일(스택)로 생성 ----------
    int leftCount = (this->field_size - this->data.size) / 2;
    int rightCount = (this->field_size - this->data.size + 1) / 2;

    // 기존 파일 내용은 지우고 새로 씀
    ofstream outL("out_of_mapL.txt", /*ios::binary | */ios::trunc);



    for (int i = 0; i < leftCount; i++) {
        recFormat rec;
        rec.obstacle = max_h - 1 - (int)sqrt(rand() % max_h*max_h + 1);
        rec.item = (rand() % 10 == 0) ? 1 : 0;
        outL << rec.obstacle << ' ' << rec.item << endl;
        //로드된 맵 왼쪽 생성, 파일에 저장
    }
    outL.close();

    // 오른쪽: i가 커질수록 bndR에 가까워지도록 설계. 동일한 이유로
    // bndR에 가장 가까운 지형이 파일 끝(top of stack)에 위치하게 됨.
    ofstream outR("out_of_mapR.txt", /*ios::binary | */ios::trunc);
    for (int i = 0; i < rightCount; i++) {
        recFormat rec;
        rec.obstacle = max_h - 1 - (int)sqrt(rand() % max_h*max_h + 1);
         rec.item = (rand() % 10 == 0) ? 1 : 0;
         outR << rec.obstacle << ' ' << rec.item << endl;
        //로드된 맵 오른쪽 생성, 파일에 저장
    }


    outR.close();

    // 각 스택(파일)에 남아있는 레코드 개수를 기록해둠
    this->out_of_loadL = leftCount;
    this->out_of_loadR = rightCount;


}

int map::load(int direction/*LEFT:0, RIGHT:1*/) {
    const char* leftFile = "out_of_mapL.txt";
    const char* rightFile = "out_of_mapR.txt";

    // direction에 따라 save 대상(반대쪽 파일)과 load 대상(이동 방향 파일)이 결정됨
    const char* saveFileName = (direction == 0) ? rightFile : leftFile;
    const char* loadFileName = (direction == 0) ? leftFile : rightFile;
    int* saveCounter = (direction == 0) ? &out_of_loadR : &out_of_loadL;
    int* loadCounter = (direction == 0) ? &out_of_loadL : &out_of_loadR;

    // 읽기/쓰기 겸용으로 열어 파일 포인터(seekg/seekp)로 위치를 직접 제어
    ofstream save(saveFileName, /*ios::binary |*/ ios::out | ios::app);
    fstream load(loadFileName, ios::in | ios::out);

    int actualSave = 0;
    int actualLoad = 0;

    Node* target;
    if (direction == 0) {
        
        target = this->data.bndR;
    }
    else {
        
        target = this->data.bndL;
    }


    // ---------------------------------------------------------
    // 1) SAVE: 이동 반대 방향 끝(bndR 또는 bndL)부터 load_unit개를 파일 스택에 push
    // ---------------------------------------------------------
    for (int i = 0; i < this->load_unit; i++) {

        recFormat rec;
        rec.obstacle = target->obstacle;
        rec.item = target->item;

        // 파일 포인터를 현재 스택 top 위치로 이동 후 기록 (push)
        save << rec.obstacle << ' ' << rec.item << endl;
        (*saveCounter)++;
        actualSave++;

        // ---------------------------------------------------------
// 2) LOAD: 이동 방향 파일의 마지막 레코드를 pop
// ---------------------------------------------------------
        if (*loadCounter > 0)
        {
            streampos startPos = 0;
            string line;

            // 마지막 줄의 시작 위치를 찾음
            while (true)
            {
                streampos pos = load.tellg();

                if (!getline(load, line))
                    break;

                startPos = pos;
            }

            // 마지막 줄의 시작 위치로 이동
            load.clear();
            load.seekg(startPos);

            // 마지막 줄의 값 읽기
            load >> target->obstacle >> target->item;

            // 마지막 줄의 시작 위치에서 앞쪽으로 이동
            load.clear();
            load.seekp(startPos - streamoff(2));

            // 마지막 줄을 공백으로 덮어쓰기
            for (size_t i = 0; i < 5; i++)
            {
                load.put(' ');
            }
            load.put('\n');

            load.flush();

            (*loadCounter)--;
            actualLoad++;
        }
        else {
            break;
        }

        if (direction == 0) {
            target = target->left;
        }
        else {
            target = target->right;
        }
    }
    save.flush();
    save.close();
    
    this->data.bndL->left = this->data.bndR;        //기존의 리스트의 끝 연결
    this->data.bndR->right = this->data.bndL;
    if (direction == 0) {
        this->data.bndR = target;       //리스트 경계 재설정
        this->data.bndL = target->right;
    }
    else {
        this->data.bndL = target;
        this->data.bndR = target->left;
    }
    this->data.bndR->right = nullptr;   //재설정된 경계 바깥 초기화
    this->data.bndL->left = nullptr;

    if (actualSave != actualLoad) {
        return -1;
    }
    return actualLoad;
}

bool map::isEnd(int x) {
    if (x > 0) {
        if (this->out_of_loadR == 0 && x == this->data.size / 2) {
            return true;
        }
    }
    else {
        if (this->out_of_loadL == 0 && -x == (this->data.size - 1) / 2) {
            return true;
        }
    }

    return false;
}

int map::getItem(int x) {
    int floor = (x > 0) ? (this->data.size / 2 - x) : ((this->data.size - 1) / 2 + x);
    Node* target = (x > 0) ? this->data.bndR : this->data.bndL;

    for (int i = 0; i < floor; i++) {
        if (x > 0) {
            target = target->left;
        }
        else {
            target = target->right;
        }
    }

    return target->item;
}

int map::isWall(int x) {
    int floor = (x > 0) ? (this->data.size / 2 - x) : ((this->data.size - 1) / 2 + x);
    Node* target = (x > 0) ? this->data.bndR : this->data.bndL;

    for (int i = 0; i < floor; i++) {
        if (x > 0) {
            target = target->left;
        }
        else {
            target = target->right;
        }
    }
    return target->obstacle;
}

map::~map() {
    Node* cur = this->data.bndL;
    while (cur != nullptr) {
        Node* next = cur->right;
        delete cur;
        cur = next;
    }

    this->data.bndL = nullptr;
    this->data.bndR = nullptr;
}
