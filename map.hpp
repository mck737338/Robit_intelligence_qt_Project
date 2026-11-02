#ifndef MAP_HPP
#define MAP_HPP

struct Node {
    int obstacle;
    int item;
    struct Node* left;
    struct Node* right;
};

struct Doubly_Linked_List {
    struct Node* bndL;
    struct Node* bndR;
    int size;
};

class map {
    int out_of_loadL;
    int out_of_loadR;
    struct Doubly_Linked_List data;
    int max_h;
    int field_size;
    int load_unit = 10;
public:
    map(int width, int height = 5, int field_size = 0, int frequency = 0);
    void reset();
    int load(int direction); // 0: LEFT, 1: RIGHT
    bool isEnd(int x);
    int getItem(int x);
    int isWall(int x);

    int getSize() const { return data.size; }
    int getFieldSize() const { return field_size; }
    int getMaxHeight() const { return max_h; }
    int getOutOfLoadL() const { return out_of_loadL; }
    int getOutOfLoadR() const { return out_of_loadR; }

    ~map();
};

#endif // MAP_HPP