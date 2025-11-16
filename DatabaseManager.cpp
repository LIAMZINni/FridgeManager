bool DatabaseManager::connectToDatabase()
{
    disconnectFromDatabase();

    qDebug() << "🔌 Connecting to PostgreSQL...";

    // Используем нового пользователя fridgeuser
    d->db = QSqlDatabase::addDatabase("QPSQL", "fridge_connection");
    d->db.setConnectOptions("connect_timeout=5");
    d->db.setHostName("localhost");
    d->db.setPort(5432);
    d->db.setDatabaseName("fridgemanager");
    d->db.setUserName("fridgeuser");
    d->db.setPassword("fridge123");  // ⭐ Новый пароль

    qDebug() << "   Host: localhost";
    qDebug() << "   Port: 5432";
    qDebug() << "   Database: fridgemanager";
    qDebug() << "   Username: fridgeuser";
    qDebug() << "   Password: ***";

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