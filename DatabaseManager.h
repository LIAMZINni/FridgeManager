#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QVector>
#include <QString>

struct ProductData {
    int id;
    QString name;
    int currentQuantity;
    int normQuantity;

    ProductData(int id = 0, const QString& name = "", int currentQty = 0, int normQty = 0)
        : id(id), name(name), currentQuantity(currentQty), normQuantity(normQty) {
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

    QVector<ProductData> getAllProducts();
    bool updateProductQuantity(int productId, int newQuantity);
    bool addProductQuantity(int productId, int amount);
    bool removeProductQuantity(int productId, int amount);

    QString getLastError() const;

private:
    // ⭐ ДОБАВЬТЕ ЭТО ОБЪЯВЛЕНИЕ
    void safeDisconnect(const QString& connectionName);
    bool verifyConnection();

    class Impl;
    Impl* d;
};

#endif // DATABASEMANAGER_H