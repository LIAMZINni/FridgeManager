#include "DatabaseManager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include <QThread>

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

    qDebug() << "🔌 Connecting to PostgreSQL (Trust Authentication)...";

    // Trust authentication - БЕЗ ПАРОЛЯ
    d->db = QSqlDatabase::addDatabase("QPSQL", "fridge_connection");
    d->db.setConnectOptions("connect_timeout=5");
    d->db.setHostName("localhost");
    d->db.setPort(5432);
    d->db.setDatabaseName("fridgemanager");
    d->db.setUserName("fridgeuser");
    d->db.setPassword("");  // ⭐ ПУСТОЙ пароль для trust auth

    qDebug() << "   Host: localhost";
    qDebug() << "   Port: 5432";
    qDebug() << "   Database: fridgemanager";
    qDebug() << "   Username: fridgeuser";
    qDebug() << "   Password: (empty - trust auth)";

    if (d->db.open()) {
        // Проверяем подключение
        QSqlQuery testQuery("SELECT version()", d->db);
        if (testQuery.exec() && testQuery.next()) {
            qDebug() << "✅ PostgreSQL:" << testQuery.value(0).toString().split(',')[0];

            // Проверяем таблицу products
            QSqlQuery tableCheck("SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = 'products')", d->db);
            if (tableCheck.exec() && tableCheck.next() && tableCheck.value(0).toBool()) {
                qDebug() << "✅ Products table exists";

                // Считаем продукты
                QSqlQuery countQuery("SELECT COUNT(*) FROM products", d->db);
                if (countQuery.exec() && countQuery.next()) {
                    qDebug() << "📊 Products count:" << countQuery.value(0).toInt();
                }

                d->connected = true;
                return true;
            }
            else {
                qWarning() << "❌ Products table not found";
            }
        }
        else {
            qWarning() << "❌ Cannot execute queries:" << testQuery.lastError().text();
        }

        d->db.close();
    }

    qWarning() << "❌ PostgreSQL connection failed:" << d->db.lastError().text();
    d->lastError = "Database connection failed";
    d->connected = false;

    // Удаляем соединение
    QSqlDatabase::removeDatabase("fridge_connection");
    return false;
}

void DatabaseManager::disconnectFromDatabase()
{
    if (d->db.isValid() && d->db.isOpen()) {
        d->db.close();
        qDebug() << "🔌 Database connection closed";
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
        qWarning() << "❌ Cannot get products: not connected to database";
        return products;
    }

    QSqlQuery query(d->db);
    QString sql = "SELECT id, name, current_quantity, norm_quantity FROM products ORDER BY id";

    qDebug() << "📋 Executing SQL:" << sql;

    if (!query.exec(sql)) {
        d->lastError = query.lastError().text();
        qWarning() << "❌ Failed to fetch products:" << d->lastError;
        return products;
    }

    int count = 0;
    while (query.next()) {
        ProductData product(
            query.value(0).toInt(),
            query.value(1).toString(),
            query.value(2).toInt(),
            query.value(3).toInt()
        );
        products.append(product);
        count++;

        qDebug() << "   Product:" << product.name
            << "Qty:" << product.currentQuantity
            << "Norm:" << product.normQuantity;
    }

    qDebug() << "✅ Loaded" << products.size() << "products from database";
    return products;
}

bool DatabaseManager::updateProductQuantity(int productId, int newQuantity)
{
    if (!isConnected()) {
        d->lastError = "Not connected to database";
        qWarning() << "❌ Cannot update product: not connected to database";
        return false;
    }

    QSqlQuery query(d->db);
    query.prepare("UPDATE products SET current_quantity = :quantity WHERE id = :id");
    query.bindValue(":quantity", newQuantity);
    query.bindValue(":id", productId);

    qDebug() << "🔄 Updating product" << productId << "to quantity" << newQuantity;

    if (!query.exec()) {
        d->lastError = query.lastError().text();
        qWarning() << "❌ Failed to update product quantity:" << d->lastError;
        return false;
    }

    bool success = query.numRowsAffected() > 0;
    if (success) {
        qDebug() << "✅ Product quantity updated successfully";
    }
    else {
        qDebug() << "⚠️ No rows affected - product might not exist";
    }

    return success;
}

bool DatabaseManager::addProductQuantity(int productId, int amount)
{
    if (!isConnected()) {
        d->lastError = "Not connected to database";
        qWarning() << "❌ Cannot add product quantity: not connected to database";
        return false;
    }

    QSqlQuery query(d->db);
    query.prepare("UPDATE products SET current_quantity = current_quantity + :amount WHERE id = :id");
    query.bindValue(":amount", amount);
    query.bindValue(":id", productId);

    qDebug() << "➕ Adding" << amount << "to product" << productId;

    if (!query.exec()) {
        d->lastError = query.lastError().text();
        qWarning() << "❌ Failed to add product quantity:" << d->lastError;
        return false;
    }

    bool success = query.numRowsAffected() > 0;
    if (success) {
        qDebug() << "✅ Product quantity added successfully";
    }
    else {
        qDebug() << "⚠️ No rows affected - product might not exist";
    }

    return success;
}

bool DatabaseManager::removeProductQuantity(int productId, int amount)
{
    if (!isConnected()) {
        d->lastError = "Not connected to database";
        qWarning() << "❌ Cannot remove product quantity: not connected to database";
        return false;
    }

    // Сначала проверим, достаточно ли товара
    QSqlQuery checkQuery(d->db);
    checkQuery.prepare("SELECT current_quantity FROM products WHERE id = :id");
    checkQuery.bindValue(":id", productId);

    qDebug() << "🔍 Checking current quantity for product" << productId;

    if (!checkQuery.exec() || !checkQuery.next()) {
        d->lastError = checkQuery.lastError().text();
        qWarning() << "❌ Failed to check product quantity:" << d->lastError;
        return false;
    }

    int currentQty = checkQuery.value(0).toInt();
    qDebug() << "   Current quantity:" << currentQty << "Requested to remove:" << amount;

    if (currentQty < amount) {
        d->lastError = "Not enough quantity available";
        qWarning() << "❌ Not enough quantity: available" << currentQty << "requested" << amount;
        return false;
    }

    // Обновляем количество
    QSqlQuery updateQuery(d->db);
    updateQuery.prepare("UPDATE products SET current_quantity = current_quantity - :amount WHERE id = :id");
    updateQuery.bindValue(":amount", amount);
    updateQuery.bindValue(":id", productId);

    qDebug() << "➖ Removing" << amount << "from product" << productId;

    if (!updateQuery.exec()) {
        d->lastError = updateQuery.lastError().text();
        qWarning() << "❌ Failed to remove product quantity:" << d->lastError;
        return false;
    }

    bool success = updateQuery.numRowsAffected() > 0;
    if (success) {
        qDebug() << "✅ Product quantity removed successfully";
    }
    else {
        qDebug() << "⚠️ No rows affected - product might not exist";
    }

    return success;
}

QString DatabaseManager::getLastError() const
{
    return d->lastError;
}