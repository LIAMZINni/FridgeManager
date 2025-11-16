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

    // 1. Peer authentication с пользователем postgres
    QString connectionName = "fridge_connection_peer";
    d->db = QSqlDatabase::addDatabase("QPSQL", connectionName);
    d->db.setConnectOptions("connect_timeout=3");
    d->db.setHostName("");  // пустой для peer auth
    d->db.setPort(-1);      // -1 для default порта
    d->db.setDatabaseName("fridgemanager");
    d->db.setUserName("postgres");
    d->db.setPassword("");

    qDebug() << "🔄 Attempting peer authentication as user 'postgres'...";

    if (d->db.open()) {
        if (verifyConnection()) {
            qDebug() << "✅ Connected via peer authentication (postgres user)";
            d->connected = true;
            return true;
        }
        safeDisconnect(connectionName);
    }
    else {
        QSqlDatabase::removeDatabase(connectionName);
    }

    // 2. Localhost подключение
    connectionName = "fridge_connection_local";
    d->db = QSqlDatabase::addDatabase("QPSQL", connectionName);
    d->db.setConnectOptions("connect_timeout=3");
    d->db.setHostName("localhost");
    d->db.setPort(5432);
    d->db.setDatabaseName("fridgemanager");
    d->db.setUserName("postgres");
    d->db.setPassword("");

    qDebug() << "🔄 Attempting localhost connection...";

    if (d->db.open()) {
        if (verifyConnection()) {
            qDebug() << "✅ Connected via localhost";
            d->connected = true;
            return true;
        }
        safeDisconnect(connectionName);
    }
    else {
        QSqlDatabase::removeDatabase(connectionName);
    }

    // 3. Socket подключение
    connectionName = "fridge_connection_socket";
    d->db = QSqlDatabase::addDatabase("QPSQL", connectionName);
    d->db.setConnectOptions("connect_timeout=3");
    d->db.setHostName("/var/run/postgresql");
    d->db.setPort(-1);
    d->db.setDatabaseName("fridgemanager");
    d->db.setUserName("postgres");
    d->db.setPassword("");

    qDebug() << "🔄 Attempting socket connection...";

    if (d->db.open()) {
        if (verifyConnection()) {
            qDebug() << "✅ Connected via socket";
            d->connected = true;
            return true;
        }
        safeDisconnect(connectionName);
    }
    else {
        QSqlDatabase::removeDatabase(connectionName);
    }

    qWarning() << "❌ All PostgreSQL connection attempts failed";
    d->lastError = "Could not establish database connection";
    d->connected = false;
    return false;
}

void DatabaseManager::safeDisconnect(const QString& connectionName)
{
    if (d->db.isValid() && d->db.isOpen()) {
        d->db.close();
    }
    // Даем время для закрытия соединения
    QThread::msleep(100);
    QSqlDatabase::removeDatabase(connectionName);
}

bool DatabaseManager::verifyConnection()
{
    if (!d->db.isOpen()) {
        qWarning() << "❌ Database not open for verification";
        return false;
    }

    // Простая проверка - пытаемся выполнить простой запрос
    QSqlQuery testQuery("SELECT 1", d->db);
    if (testQuery.exec() && testQuery.next()) {
        qDebug() << "✅ Basic connection test passed";

        // Проверяем существование таблицы products
        QSqlQuery tableCheck("SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = 'products')", d->db);
        if (tableCheck.exec() && tableCheck.next()) {
            bool tableExists = tableCheck.value(0).toBool();
            if (tableExists) {
                qDebug() << "✅ Products table exists";
                return true;
            }
            else {
                qWarning() << "❌ Products table does not exist";
                return false;
            }
        }
        else {
            qWarning() << "❌ Cannot check products table";
            return false;
        }
    }
    else {
        qWarning() << "❌ Basic connection test failed";
        return false;
    }
}

void DatabaseManager::disconnectFromDatabase()
{
    if (d->db.isValid() && d->db.isOpen()) {
        QString connectionName = d->db.connectionName();
        d->db.close();
        safeDisconnect(connectionName);
    }
    d->connected = false;
}

bool DatabaseManager::isConnected() const
{
    return d->connected && d->db.isValid() && d->db.isOpen();
}

// Остальные методы остаются без изменений...
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