#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>

struct Product {
    int id;
    std::string name;
    int currentQuantity;
    int normQuantity;

    Product(int i = 0, const std::string& n = "", int curr = 0, int norm = 0)
        : id(i), name(n), currentQuantity(curr), normQuantity(norm) {
    }
};

#endif