#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <locale>
#include <ctime>
#include "DatabaseManager.h"
#include "Product.h"  // Добавляем включение Product

#ifdef _WIN32
#include <windows.h>
#endif

class FridgeManager {
private:
    std::vector<Product> products;
    DatabaseManager dbManager;
    bool useDatabase;

public:
    FridgeManager() : useDatabase(false) {
        // Пытаемся подключиться к базе данных
        if (dbManager.connect("localhost", "5432", "fridgemanager", "postgres", "123")) {
            std::cout << "Подключение к базе данных успешно!" << std::endl;
            useDatabase = true;
            loadFromDatabase();
        }
        else {
            std::cout << "Используется локальное хранилище (база данных недоступна)" << std::endl;
            // Резервные данные в памяти
            initializeDefaultProducts();
        }
    }

    void showProducts() {
        std::cout << "\n=== ОСТАТКИ В ХОЛОДИЛЬНИКЕ ===" << std::endl;
        for (size_t i = 0; i < products.size(); ++i) {
            std::cout << i + 1 << ". " << products[i].name
                << " | В наличии: " << products[i].currentQuantity
                << " | Норма: " << products[i].normQuantity;

            if (products[i].currentQuantity < products[i].normQuantity) {
                std::cout << " | Нужен заказ: " << (products[i].normQuantity - products[i].currentQuantity);
            }
            else {
                std::cout << " | Достаточно";
            }
            std::cout << std::endl;
        }
    }

    void addProductQuantity(int index, int amount) {
        if (index >= 0 && index < products.size()) {
            if (useDatabase) {
                if (dbManager.addProductQuantity(products[index].id, amount)) {
                    products[index].currentQuantity += amount;
                    std::cout << "Добавлено " << amount << " упаковок " << products[index].name << std::endl;
                }
                else {
                    std::cout << "Ошибка при обновлении базы данных!" << std::endl;
                }
            }
            else {
                products[index].currentQuantity += amount;
                std::cout << "Добавлено " << amount << " упаковок " << products[index].name << std::endl;
            }
        }
        else {
            std::cout << "Ошибка: неверный индекс продукта!" << std::endl;
        }
    }

    void removeProductQuantity(int index, int amount) {
        if (index >= 0 && index < products.size()) {
            if (products[index].currentQuantity >= amount) {
                if (useDatabase) {
                    if (dbManager.removeProductQuantity(products[index].id, amount)) {
                        products[index].currentQuantity -= amount;
                        std::cout << "Израсходовано " << amount << " упаковок " << products[index].name << std::endl;
                    }
                    else {
                        std::cout << "Ошибка при обновлении базы данных!" << std::endl;
                    }
                }
                else {
                    products[index].currentQuantity -= amount;
                    std::cout << "Израсходовано " << amount << " упаковок " << products[index].name << std::endl;
                }
            }
            else {
                std::cout << "Ошибка: недостаточно " << products[index].name << " в холодильнике!" << std::endl;
            }
        }
        else {
            std::cout << "Ошибка: неверный индекс продукта!" << std::endl;
        }
    }

    void generateOrder() {
        std::cout << "\n=== ЗАЯВКА ДЛЯ ПОСТАВЩИКА ===" << std::endl;
        bool hasOrders = false;

        for (const auto& product : products) {
            if (product.currentQuantity < product.normQuantity) {
                int orderQty = product.normQuantity - product.currentQuantity;
                std::cout << product.name << ": " << orderQty << " упаковок" << std::endl;
                hasOrders = true;
            }
        }

        if (!hasOrders) {
            std::cout << "Все продукты в достаточном количестве!" << std::endl;
        }

        // Сохраняем в файл
        saveOrderToFile();
    }

    void saveOrderToFile() {
        std::ofstream file("заявка_поставщику.txt");
        if (file.is_open()) {
            file << "Заявка для поставщика\n";
            file << "=====================\n";
            file << "Дата: " << getCurrentDate() << "\n\n";

            bool hasOrders = false;
            for (const auto& product : products) {
                if (product.currentQuantity < product.normQuantity) {
                    int orderQty = product.normQuantity - product.currentQuantity;
                    file << product.name << ": " << orderQty << " упаковок\n";
                    hasOrders = true;
                }
            }

            if (!hasOrders) {
                file << "Все продукты в достаточном количестве.\n";
            }

            file.close();
            std::cout << "Заявка сохранена в файл 'заявка_поставщику.txt'" << std::endl;
        }
        else {
            std::cout << "Ошибка при создании файла заявки!" << std::endl;
        }
    }

    void showMenu() {
        std::cout << "\n=== УЧЕТ ПРОДУКТОВ РЕСТОРАНА ===" << std::endl;
        std::cout << "1. Показать остатки" << std::endl;
        std::cout << "2. Добавить продукт (приход)" << std::endl;
        std::cout << "3. Израсходовать продукт (расход)" << std::endl;
        std::cout << "4. Сформировать заявку" << std::endl;
        std::cout << "5. Обновить данные из базы" << std::endl;
        std::cout << "6. Выход" << std::endl;
        std::cout << "Выберите действие: ";
    }

    void run() {
        int choice;

        while (true) {
            showMenu();
            std::cin >> choice;

            switch (choice) {
            case 1:
                showProducts();
                break;

            case 2: {
                showProducts();
                std::cout << "Выберите продукт (номер): ";
                int index;
                std::cin >> index;
                std::cout << "Количество: ";
                int amount;
                std::cin >> amount;
                addProductQuantity(index - 1, amount);
                break;
            }

            case 3: {
                showProducts();
                std::cout << "Выберите продукт (номер): ";
                int index;
                std::cin >> index;
                std::cout << "Количество: ";
                int amount;
                std::cin >> amount;
                removeProductQuantity(index - 1, amount);
                break;
            }

            case 4:
                generateOrder();
                break;

            case 5:
                if (useDatabase) {
                    loadFromDatabase();
                    std::cout << "Данные обновлены из базы данных!" << std::endl;
                }
                else {
                    std::cout << "База данных недоступна!" << std::endl;
                }
                break;

            case 6:
                std::cout << "Выход из программы..." << std::endl;
                return;

            default:
                std::cout << "Неверный выбор!" << std::endl;
            }
        }
    }

private:
    void initializeDefaultProducts() {
        products.clear();
        products.push_back(Product(1, "Творог", 5, 10));
        products.push_back(Product(2, "Сыр", 12, 15));
        products.push_back(Product(3, "Молоко", 18, 20));
        products.push_back(Product(4, "Яйца", 25, 30));
        products.push_back(Product(5, "Оливки", 3, 8));
    }

    void loadFromDatabase() {
        if (useDatabase) {
            auto dbProducts = dbManager.getAllProducts();
            products = dbProducts; // Просто присваиваем вектор
        }
    }

    std::string getCurrentDate() {
        time_t now = time(0);
        tm* localTime = localtime(&now);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%d.%m.%Y", localTime);
        return std::string(buffer);
    }
};

void setRussianEncoding() {
#ifdef _WIN32
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
#endif
    std::locale::global(std::locale(""));
}

int main() {
    setRussianEncoding();

    FridgeManager manager;
    manager.run();
    return 0;
}