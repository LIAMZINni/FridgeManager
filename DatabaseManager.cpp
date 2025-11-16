#include "DatabaseManager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include <QCoreApplication>  // ⭐ ДОБАВЬТЕ ЭТОТ INCLUDE

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

    // 🔄 Пробуем разные методы подключения в порядке приоритета

    // 1. Peer authentication с текущим системным пользователем
    QString currentUser = qgetenv("USER");
    if (currentUser.isEmpty()) {
        currentUser = "postgres";
    }

    qDebug() << "👤 Current system user:" << currentUser;

    // Метод 1: Peer authentication (самый надежный)
    d->db = QSqlDatabase::addDatabase("QPSQL", "fridge_connection_peer");
    d->db.setConnectOptions("connect_timeout=3");
    d->db.setHostName("");  // пустой для peer auth
    d->db.setPort(-1);      // -1 для default порта
    d->db.setDatabaseName("fridgemanager");
    d->db.setUserName(currentUser);  // используем текущего системного пользователя
    d->db.setPassword("");

    qDebug() << "🔄 Attempting peer authentication as user:" << currentUser;

    if (d->db.open()) {
        if (verifyConnection()) {
            qDebug() << "✅ Connected via peer authentication";
            d->connected = true;
            return true;
        }
        d->db.close();
    }
    else {
        qDebug() << "❌ Peer auth failed:" << d->db.lastError().text();
    }
    QSqlDatabase::removeDatabase("fridge_connection_peer");

    // Метод 2: Peer authentication с пользователем postgres
    d->db = QSqlDatabase::addDatabase("QPSQL", "fridge_connection_postgres");
    d->db.setConnectOptions("connect_timeout=3");
    d->db.setHostName("");
    d->db.setPort(-1);
    d->db.setDatabaseName("fridgemanager");
    d->db.setUserName("postgres");
    d->db.setPassword("");

    qDebug() << "🔄 Attempting peer authentication as user: postgres";

    if (d->db.open()) {
        if (verifyConnection()) {
            qDebug() << "✅ Connected via peer authentication (postgres)";
            d->connected = true;
            return true;
        }
        d->db.close();
    }
    else {
        qDebug() << "❌ Peer auth (postgres) failed:" << d->db.lastError().text();
    }
    QSqlDatabase::removeDatabase("fridge_connection_postgres");

    // Метод 3: Localhost подключение
    d->db = QSqlDatabase::addDatabase("QPSQL", "fridge_connection_local");
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
        d->db.close();
    }
    else {
        qDebug() << "❌ Localhost connection failed:" << d->db.lastError().text();
    }
    QSqlDatabase::removeDatabase("fridge_connection_local");

    qWarning() << "❌ All PostgreSQL connection attempts failed";
    d->lastError = "Could not establish database connection";
    d->connected = false;
    return false;
}

// ⭐ ВАЖНО: РЕАЛИЗАЦИЯ ФУНКЦИИ ДОЛЖНА БЫТЬ ПОСЛЕ connectToDatabase()
bool DatabaseManager::verifyConnection()
{
    if (!d->db.isOpen()) {
        qDebug() << "❌ Database not open in verifyConnection";
        return false;
    }

    // ⭐ ВАЖНО: Используем простой и надежный запрос
    // Не используем сложные запросы которые могут вызвать ошибки

    // Простая проверка версии PostgreSQL
    QSqlQuery testQuery(d->db);
    if (!testQuery.exec("SELECT 1")) {
        qWarning() << "❌ Simple test query failed:" << testQuery.lastError().text();
        return false;
    }

    if (testQuery.next()) {
        qDebug() << "✅ Basic connection test passed";
    }
    else {
        qWarning() << "❌ No results from test query";
        return false;
    }

    // Проверяем существование таблицы products (простой запрос)
    QSqlQuery tableQuery(d->db);
    if (!tableQuery.exec("SELECT COUNT(*) FROM products")) {
        qDebug() << "⚠️ Products table check failed (might not exist):" << tableQuery.lastError().text();

        // Если таблицы нет, это не критично - приложение создаст локальные данные
        qDebug() << "📋 Will use local data mode";
        return true; // Все равно возвращаем true, так как подключение работает
    }

    if (tableQuery.next()) {
        int productCount = tableQuery.value(0).toInt();
        qDebug() << "✅ Products table exists, count:" << productCount;
    }

    return true;
}

void DatabaseManager::disconnectFromDatabase()
{
    if (d->db.isValid() && d->db.isOpen()) {
        d->db.close();
        qDebug() << "🔌 Database connection closed";
    }
    d->connected = false;
    // ⭐ УБЕРИТЕ ЭТУ СТРОКУ: QCoreApplication::processEvents();
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