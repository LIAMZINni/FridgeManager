#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>

struct ProductData {
    int id;
    QString name;
    int currentQuantity;
    int normQuantity;

    ProductData(int i = 0, const QString& n = "", int curr = 0, int norm = 0)
        : id(i), name(n), currentQuantity(curr), normQuantity(norm) {
    }
};

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    bool connectToDatabase();
    void disconnectFromDatabase();
    bool isConnected() const;

    // Операции с продуктами
    QVector<ProductData> getAllProducts();
    bool updateProductQuantity(int productId, int newQuantity);
    bool addProductQuantity(int productId, int amount);
    bool removeProductQuantity(int productId, int amount);

    QString getLastError() const;

private:
    class Impl;
    Impl* d;
};

#endif