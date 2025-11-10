#include "DatabaseManager.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

class DatabaseManager::Impl
{
public:
    QSqlDatabase db;
    QString lastError;
    bool connected = false;
};

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
    , d(new Impl())
{
}

DatabaseManager::~DatabaseManager()
{
    disconnectFromDatabase();
    delete d;
}

bool DatabaseManager::connectToDatabase()
{
    disconnectFromDatabase();

    // Создаем подключение
    d->db = QSqlDatabase::addDatabase("QPSQL", "fridge_connection");
    d->db.setHostName("localhost");
    d->db.setPort(5432);
    d->db.setDatabaseName("fridgemanager");
    d->db.setUserName("postgres");
    d->db.setPassword("123"); 

    if (!d->db.open()) {
        d->lastError = d->db.lastError().text();
        qWarning() << "Failed to connect to PostgreSQL:" << d->lastError;
        d->connected = false;
        return false;
    }

    qDebug() << "Successfully connected to PostgreSQL database";
    d->connected = true;
    return true;
}

void DatabaseManager::disconnectFromDatabase()
{
    if (d->db.isValid() && d->db.isOpen()) {
        d->db.close();
    }
    d->connected = false;
    QSqlDatabase::removeDatabase("fridge_connection");
}

bool DatabaseManager::isConnected() const
{
    return d->connected && d->db.isValid() && d->db.isOpen();
}

QVector<ProductData> DatabaseManager::getAllProducts()
{
    QVector<ProductData> products;

    if (!isConnected()) {
        d->lastError = "Not connected to database";
        return products;
    }

    QSqlQuery query(d->db);
    QString sql = "SELECT id, name, current_quantity, norm_quantity FROM products ORDER BY id";

    if (!query.exec(sql)) {
        d->lastError = query.lastError().text();
        qWarning() << "Failed to fetch products:" << d->lastError;
        return products;
    }

    while (query.next()) {
        ProductData product(
            query.value(0).toInt(),
            query.value(1).toString(),
            query.value(2).toInt(),
            query.value(3).toInt()
        );
        products.append(product);
    }

    qDebug() << "Loaded" << products.size() << "products from database";
    return products;
}

bool DatabaseManager::updateProductQuantity(int productId, int newQuantity)
{
    if (!isConnected()) {
        d->lastError = "Not connected to database";
        return false;
    }

    QSqlQuery query(d->db);
    query.prepare("UPDATE products SET current_quantity = :quantity WHERE id = :id");
    query.bindValue(":quantity", newQuantity);
    query.bindValue(":id", productId);

    if (!query.exec()) {
        d->lastError = query.lastError().text();
        qWarning() << "Failed to update product quantity:" << d->lastError;
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool DatabaseManager::addProductQuantity(int productId, int amount)
{
    if (!isConnected()) {
        d->lastError = "Not connected to database";
        return false;
    }

    QSqlQuery query(d->db);
    query.prepare("UPDATE products SET current_quantity = current_quantity + :amount WHERE id = :id");
    query.bindValue(":amount", amount);
    query.bindValue(":id", productId);

    if (!query.exec()) {
        d->lastError = query.lastError().text();
        qWarning() << "Failed to add product quantity:" << d->lastError;
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool DatabaseManager::removeProductQuantity(int productId, int amount)
{
    if (!isConnected()) {
        d->lastError = "Not connected to database";
        return false;
    }

    // Сначала проверим, достаточно ли товара
    QSqlQuery checkQuery(d->db);
    checkQuery.prepare("SELECT current_quantity FROM products WHERE id = :id");
    checkQuery.bindValue(":id", productId);

    if (!checkQuery.exec() || !checkQuery.next()) {
        d->lastError = checkQuery.lastError().text();
        qWarning() << "Failed to check product quantity:" << d->lastError;
        return false;
    }

    int currentQty = checkQuery.value(0).toInt();
    if (currentQty < amount) {
        d->lastError = "Not enough quantity available";
        return false;
    }

    // Обновляем количество
    QSqlQuery updateQuery(d->db);
    updateQuery.prepare("UPDATE products SET current_quantity = current_quantity - :amount WHERE id = :id");
    updateQuery.bindValue(":amount", amount);
    updateQuery.bindValue(":id", productId);

    if (!updateQuery.exec()) {
        d->lastError = updateQuery.lastError().text();
        qWarning() << "Failed to remove product quantity:" << d->lastError;
        return false;
    }

    return updateQuery.numRowsAffected() > 0;
}

QString DatabaseManager::getLastError() const
{
    return d->lastError;
}