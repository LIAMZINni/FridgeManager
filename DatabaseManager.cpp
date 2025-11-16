#include "DatabaseManager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>

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

    qDebug() << "🔌 Starting PostgreSQL connection attempts...";

    // 🔄 Пробуем разные методы подключения

    // 1. Peer authentication с пользователем postgres
    d->db = QSqlDatabase::addDatabase("QPSQL", "fridge_connection");
    d->db.setConnectOptions("connect_timeout=5");
    d->db.setHostName("localhost");
    d->db.setPort(5432);
    d->db.setDatabaseName("fridgemanager");
    d->db.setUserName("postgres");
    d->db.setPassword("");

    qDebug() << "🔄 Attempting PostgreSQL connection...";
    qDebug() << "   Host:" << d->db.hostName();
    qDebug() << "   Port:" << d->db.port();
    qDebug() << "   Database:" << d->db.databaseName();
    qDebug() << "   Username:" << d->db.userName();

    if (d->db.open()) {
        qDebug() << "✅ Database opened successfully";

        if (verifyConnection()) {
            qDebug() << "🎉 Database connection established and verified";
            d->connected = true;
            return true;
        }
        else {
            qWarning() << "❌ Connection verification failed";
            d->db.close();
        }
    }
    else {
        qWarning() << "❌ Failed to open database:" << d->db.lastError().text();
    }

    // Если первый способ не сработал, пробуем peer authentication
    QSqlDatabase::removeDatabase("fridge_connection");

    d->db = QSqlDatabase::addDatabase("QPSQL", "fridge_connection_peer");
    d->db.setConnectOptions("connect_timeout=5");
    d->db.setHostName("");  // пустой для peer auth
    d->db.setPort(-1);      // -1 для default порта
    d->db.setDatabaseName("fridgemanager");
    d->db.setUserName("postgres");
    d->db.setPassword("");

    qDebug() << "🔄 Attempting peer authentication...";

    if (d->db.open()) {
        qDebug() << "✅ Database opened via peer auth";

        if (verifyConnection()) {
            qDebug() << "🎉 Peer authentication connection established";
            d->connected = true;
            return true;
        }
        else {
            d->db.close();
        }
    }

    qWarning() << "❌ All PostgreSQL connection attempts failed";
    d->lastError = "Could not establish database connection";
    d->connected = false;
    QSqlDatabase::removeDatabase("fridge_connection_peer");
    return false;
}

bool DatabaseManager::verifyConnection()
{
    if (!d->db.isOpen()) {
        qWarning() << "❌ Database is not open";
        return false;
    }

    // Простой тестовый запрос
    QSqlQuery testQuery(d->db);
    if (!testQuery.exec("SELECT 1")) {
        qWarning() << "❌ Simple test query failed:" << testQuery.lastError().text();
        return false;
    }

    if (testQuery.next()) {
        qDebug() << "✅ Basic query execution: OK";
    }
    else {
        qWarning() << "❌ No results from test query";
        return false;
    }

    // Проверяем версию PostgreSQL
    QSqlQuery versionQuery(d->db);
    if (versionQuery.exec("SELECT version()") && versionQuery.next()) {
        QString version = versionQuery.value(0).toString();
        qDebug() << "✅ PostgreSQL version:" << version.split(',')[0];
    }
    else {
        qWarning() << "⚠️ Cannot get PostgreSQL version:" << versionQuery.lastError().text();
    }

    // Проверяем существование таблицы products
    QSqlQuery tableCheck(d->db);
    if (tableCheck.exec("SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = 'products')")) {
        if (tableCheck.next()) {
            bool tableExists = tableCheck.value(0).toBool();
            if (tableExists) {
                qDebug() << "✅ Products table exists";

                // Проверяем количество продуктов
                QSqlQuery countQuery(d->db);
                if (countQuery.exec("SELECT COUNT(*) FROM products") && countQuery.next()) {
                    qDebug() << "   Products count:" << countQuery.value(0).toInt();
                }
                return true;
            }
            else {
                qWarning() << "❌ Products table does not exist";
                return false;
            }
        }
    }
    else {
        qWarning() << "❌ Cannot check products table:" << tableCheck.lastError().text();
        return false;
    }

    return true;
}

void DatabaseManager::disconnectFromDatabase()
{
    if (d->db.isValid() && d->db.isOpen()) {
        QString connectionName = d->db.connectionName();
        d->db.close();
        qDebug() << "🔌 Database connection closed:" << connectionName;

        // Даем время на закрытие соединения перед удалением
        QCoreApplication::processEvents();
        QSqlDatabase::removeDatabase(connectionName);
    }
    d->connected = false;
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