#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

#include "DatabaseManager.h"  // Добавьте этот include

class Product : public QObject
{
    Q_OBJECT
        Q_PROPERTY(int id READ id CONSTANT)
        Q_PROPERTY(QString name READ name CONSTANT)
        Q_PROPERTY(int currentQuantity READ currentQuantity WRITE setCurrentQuantity NOTIFY currentQuantityChanged)
        Q_PROPERTY(int normQuantity READ normQuantity CONSTANT)
        Q_PROPERTY(bool needsOrder READ needsOrder NOTIFY currentQuantityChanged)
        Q_PROPERTY(int orderQuantity READ orderQuantity NOTIFY currentQuantityChanged)

public:
    Product(QObject* parent = nullptr) : QObject(parent) {}
    Product(int id, const QString& name, int currentQty, int normQty, QObject* parent = nullptr)
        : QObject(parent), m_id(id), m_name(name), m_currentQuantity(currentQty), m_normQuantity(normQty) {
    }

    int id() const { return m_id; }
    QString name() const { return m_name; }
    int currentQuantity() const { return m_currentQuantity; }
    int normQuantity() const { return m_normQuantity; }
    bool needsOrder() const { return m_currentQuantity < m_normQuantity; }
    int orderQuantity() const { return qMax(0, m_normQuantity - m_currentQuantity); }

    void setCurrentQuantity(int quantity) {
        if (m_currentQuantity != quantity) {
            m_currentQuantity = quantity;
            emit currentQuantityChanged();
        }
    }

signals:
    void currentQuantityChanged();

private:
    int m_id = 0;
    QString m_name;
    int m_currentQuantity = 0;
    int m_normQuantity = 0;
};

class FridgeManager : public QObject
{
    Q_OBJECT
        Q_PROPERTY(QQmlListProperty<Product> products READ products NOTIFY productsChanged)
        Q_PROPERTY(bool databaseConnected READ databaseConnected NOTIFY databaseStatusChanged)
        Q_PROPERTY(QString databaseStatus READ databaseStatus NOTIFY databaseStatusChanged)
        Q_PROPERTY(QString lastSavePath READ lastSavePath NOTIFY lastSavePathChanged)

public:
    explicit FridgeManager(QObject* parent = nullptr)
        : QObject(parent)
        , m_databaseConnected(false)
        , m_databaseStatus("Подключение к БД...")
        , m_lastSavePath("")
    {
        initializeDatabase();
    }

    QQmlListProperty<Product> products() {
        return QQmlListProperty<Product>(this, &m_products);
    }

    bool databaseConnected() const { return m_databaseConnected; }
    QString databaseStatus() const { return m_databaseStatus; }
    QString lastSavePath() const { return m_lastSavePath; }

    Q_INVOKABLE void addProductQuantity(int index, int amount) {
        if (index >= 0 && index < m_products.size()) {
            Product* product = m_products[index];

            if (m_databaseConnected) {
                // Обновляем в базе данных
                if (m_dbManager.addProductQuantity(product->id(), amount)) {
                    product->setCurrentQuantity(product->currentQuantity() + amount);
                }
            }
            else {
                // Локальное обновление
                product->setCurrentQuantity(product->currentQuantity() + amount);
            }
            emit productsChanged();
        }
    }

    Q_INVOKABLE void removeProductQuantity(int index, int amount) {
        if (index >= 0 && index < m_products.size()) {
            Product* product = m_products[index];

            if (product->currentQuantity() >= amount) {
                if (m_databaseConnected) {
                    // Обновляем в базе данных
                    if (m_dbManager.removeProductQuantity(product->id(), amount)) {
                        product->setCurrentQuantity(product->currentQuantity() - amount);
                    }
                }
                else {
                    // Локальное обновление
                    product->setCurrentQuantity(product->currentQuantity() - amount);
                }
                emit productsChanged();
            }
        }
    }

    Q_INVOKABLE QString generateOrder() {
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString defaultFileName = defaultPath + "/заявка_поставщику_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".txt";

        QString result = saveOrderToFile(defaultFileName);
        if (result.startsWith("Успех:")) {
            m_lastSavePath = defaultFileName;
            emit lastSavePathChanged();
        }
        return result;
    }

    Q_INVOKABLE QString saveOrderToPath(const QString& directoryPath) {
        QString fileName = directoryPath + "/заявка_поставщику_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".txt";

        QString result = saveOrderToFile(fileName);
        if (result.startsWith("Успех:")) {
            m_lastSavePath = fileName;
            emit lastSavePathChanged();
        }
        return result;
    }

    Q_INVOKABLE QString getDefaultDocumentsPath() {
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    Q_INVOKABLE QString getDefaultDownloadsPath() {
        return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }

    Q_INVOKABLE QString getDefaultHomePath() {
        return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    Q_INVOKABLE QString getDesktopPath() {
        return QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }

    Q_INVOKABLE bool fileExists(const QString& filePath) {
        return QFile::exists(filePath);
    }

    Q_INVOKABLE bool directoryExists(const QString& dirPath) {
        return QDir(dirPath).exists();
    }

    Q_INVOKABLE bool createDirectory(const QString& dirPath) {
        return QDir().mkpath(dirPath);
    }

    Q_INVOKABLE QStringList getAvailableDirectories() {
        QStringList dirs;
        dirs << getDefaultHomePath() + "/Заявки";
        dirs << getDesktopPath();
        dirs << getDefaultDocumentsPath();
        dirs << getDefaultDownloadsPath();
        dirs << QDir::currentPath() + "/заявки";

        QStringList availableDirs;
        for (const QString& dir : dirs) {
            if (QDir(dir).exists() || QDir().mkpath(dir)) {
                availableDirs << dir;
            }
        }

        return availableDirs;
    }

signals:
    void productsChanged();
    void databaseStatusChanged();
    void lastSavePathChanged();

private:
    void initializeDatabase() {
        // Пытаемся подключиться к PostgreSQL
        if (m_dbManager.connectToDatabase() && m_dbManager.isConnected()) {
            m_databaseConnected = true;
            m_databaseStatus = "✅ База данных PostgreSQL подключена";

            // Загружаем продукты из БД
            auto productsData = m_dbManager.getAllProducts();
            if (!productsData.isEmpty()) {
                loadProductsFromDatabase(productsData);
                qDebug() << "Загружено продуктов из БД:" << productsData.size();
            }
            else {
                m_databaseStatus = "❌ БД подключена, но продукты не найдены";
                initializeLocalProducts();
            }
        }
        else {
            // Если БД недоступна - локальный режим
            m_databaseConnected = false;
            m_databaseStatus = "📋 Локальный режим (БД недоступна)";
            qDebug() << "PostgreSQL недоступна, ошибка:" << m_dbManager.getLastError();
            initializeLocalProducts();
        }
        emit databaseStatusChanged();
    }

    void loadProductsFromDatabase(const QVector<ProductData>& productsData) {
        m_products.clear();
        for (const auto& productData : productsData) {
            m_products.append(new Product(
                productData.id,
                productData.name,
                productData.currentQuantity,
                productData.normQuantity,
                this
            ));
        }
    }

    void initializeLocalProducts() {
        m_products.clear();

        m_products.append(new Product(1, "Творог", 5, 10, this));
        m_products.append(new Product(2, "Сыр", 12, 15, this));
        m_products.append(new Product(3, "Молоко", 18, 20, this));
        m_products.append(new Product(4, "Яйца", 25, 30, this));
        m_products.append(new Product(5, "Оливки", 3, 8, this));
    }

    QString saveOrderToFile(const QString& filePath) {
        QFileInfo fileInfo(filePath);
        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                return "Ошибка: Не удалось создать директорию " + dir.absolutePath();
            }
        }

        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            stream.setCodec("UTF-8");
#else
            stream.setEncoding(QStringConverter::Utf8);
#endif

            stream << "ЗАЯВКА ДЛЯ ПОСТАВЩИКА\n";
            stream << "=====================\n";
            stream << "Ресторан: 'Гурман'\n";
            stream << "Дата: " << QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm") << "\n";
            stream << "Статус БД: " << m_databaseStatus << "\n";
            stream << "=====================\n\n";

            bool hasOrders = false;
            int totalPacks = 0;

            stream << "ПРОДУКТЫ ДЛЯ ЗАКАЗА:\n";
            stream << "-------------------\n";

            for (Product* product : m_products) {
                if (product->needsOrder()) {
                    int orderQty = product->orderQuantity();
                    stream << "• " << product->name() << ": " << orderQty << " упаковок\n";
                    hasOrders = true;
                    totalPacks += orderQty;
                }
            }

            if (!hasOrders) {
                stream << "Все продукты в достаточном количестве.\n";
            }
            else {
                stream << "\n-------------------\n";
                stream << "ИТОГО: " << totalPacks << " упаковок\n";
            }

            // Текущие остатки
            stream << "\n=====================\n";
            stream << "ТЕКУЩИЕ ОСТАТКИ:\n";
            stream << "-------------------\n";

            for (Product* product : m_products) {
                stream << "• " << product->name() << ": " << product->currentQuantity()
                    << " / " << product->normQuantity() << " упаковок";
                if (product->needsOrder()) {
                    stream << " (нужно " << product->orderQuantity() << ")";
                }
                stream << "\n";
            }

            file.close();

            qDebug() << "Order saved to:" << filePath;
            return "Успех: Заявка сохранена в " + filePath;
        }
        else {
            qDebug() << "Failed to save order file:" << filePath;
            return "Ошибка: Не удалось сохранить файл " + filePath;
        }
    }

    QList<Product*> m_products;
    DatabaseManager m_dbManager;  // Добавьте этот член
    bool m_databaseConnected;
    QString m_databaseStatus;
    QString m_lastSavePath;
};

// ... остальная часть main.cpp без изменений