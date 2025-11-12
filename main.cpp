#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include "DatabaseManager.h"

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

public:
    explicit FridgeManager(QObject* parent = nullptr)
        : QObject(parent)
        , m_dbManager(new DatabaseManager(this))
    {
        initializeDatabase();
    }

    QQmlListProperty<Product> products() {
        return QQmlListProperty<Product>(this, &m_products);
    }

    bool databaseConnected() const { return m_databaseConnected; }
    QString databaseStatus() const { return m_databaseStatus; }

    Q_INVOKABLE void addProductQuantity(int index, int amount) {
        if (index >= 0 && index < m_products.size()) {
            Product* product = m_products[index];
            int newQuantity = product->currentQuantity() + amount;

            if (m_databaseConnected) {
                if (m_dbManager->updateProductQuantity(product->id(), newQuantity)) {
                    product->setCurrentQuantity(newQuantity);
                    emit productsChanged();
                }
                else {
                    qWarning() << "Database update failed:" << m_dbManager->getLastError();
                    // Все равно обновляем локально
                    product->setCurrentQuantity(newQuantity);
                    emit productsChanged();
                }
            }
            else {
                product->setCurrentQuantity(newQuantity);
                emit productsChanged();
            }
        }
    }

    Q_INVOKABLE void removeProductQuantity(int index, int amount) {
        if (index >= 0 && index < m_products.size()) {
            Product* product = m_products[index];

            if (product->currentQuantity() >= amount) {
                int newQuantity = product->currentQuantity() - amount;

                if (m_databaseConnected) {
                    if (m_dbManager->updateProductQuantity(product->id(), newQuantity)) {
                        product->setCurrentQuantity(newQuantity);
                        emit productsChanged();
                    }
                    else {
                        qWarning() << "Database update failed:" << m_dbManager->getLastError();
                        // Все равно обновляем локально
                        product->setCurrentQuantity(newQuantity);
                        emit productsChanged();
                    }
                }
                else {
                    product->setCurrentQuantity(newQuantity);
                    emit productsChanged();
                }
            }
        }
    }

    Q_INVOKABLE void refreshProducts() {
        loadProductsFromDatabase();
    }

    Q_INVOKABLE QString generateOrder(const QString& filePath) {
        qDebug() << "Generating order to:" << filePath;

        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            stream.setEncoding(QStringConverter::Utf8);  // Qt6
#else
            stream.setCodec("UTF-8");  // Qt5
#endif

            // Заголовок заявки
            stream << "ЗАЯВКА ДЛЯ ПОСТАВЩИКА\n";
            stream << "=====================\n";
            stream << "Ресторан: 'Гурман'\n";
            stream << "Дата: " << QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm") << "\n";
            stream << "=====================\n\n";

            bool hasOrders = false;
            int totalPacks = 0;

            // Продукты для заказа
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

            file.close();

            qDebug() << "Order successfully saved to:" << filePath;
            return "Успех: Заявка сохранена в " + filePath;
        }
        else {
            qDebug() << "Failed to save order file!";
            return "Ошибка: Не удалось сохранить файл";
        }
    }

    Q_INVOKABLE QString getDefaultFileName() {
        return "заявка_поставщику_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".txt";
    }

    Q_INVOKABLE QString getDocumentsPath() {
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

signals:
    void productsChanged();
    void databaseStatusChanged();

private:
    void initializeDatabase() {
        m_databaseConnected = m_dbManager->connectToDatabase();

        if (m_databaseConnected) {
            m_databaseStatus = "PostgreSQL подключена";
            qDebug() << "Database connected successfully";
            loadProductsFromDatabase();
        }
        else {
            m_databaseStatus = "Локальный режим (PostgreSQL недоступна)";
            qWarning() << "Database connection failed, using local mode";
            initializeLocalProducts();
        }

        emit databaseStatusChanged();
    }

    void loadProductsFromDatabase() {
        if (m_databaseConnected) {
            auto dbProducts = m_dbManager->getAllProducts();
            if (!dbProducts.isEmpty()) {
                updateProductsList(dbProducts);
                return;
            }
        }

        // Если БД недоступна или пуста - используем локальные данные
        initializeLocalProducts();
    }

    void updateProductsList(const QVector<ProductData>& dbProducts) {
        qDeleteAll(m_products);
        m_products.clear();

        for (const auto& productData : dbProducts) {
            m_products.append(new Product(
                productData.id,
                productData.name,
                productData.currentQuantity,
                productData.normQuantity,
                this
            ));
        }

        emit productsChanged();
        qDebug() << "Products loaded from database:" << m_products.size();
    }

    void initializeLocalProducts() {
        qDeleteAll(m_products);
        m_products.clear();

        m_products.append(new Product(1, "Творог", 5, 10, this));
        m_products.append(new Product(2, "Сыр", 12, 15, this));
        m_products.append(new Product(3, "Молоко", 18, 20, this));
        m_products.append(new Product(4, "Яйца", 25, 30, this));
        m_products.append(new Product(5, "Оливки", 3, 8, this));

        emit productsChanged();
        qDebug() << "Using local products data";
    }

    DatabaseManager* m_dbManager;
    QList<Product*> m_products;
    bool m_databaseConnected = false;
    QString m_databaseStatus = "Подключение...";
};

int main(int argc, char* argv[])
{
    qDebug() << "Starting application...";

    QGuiApplication app(argc, argv);

    app.setApplicationName("FridgeManager");
    app.setApplicationVersion("1.0.0");

    qmlRegisterType<Product>("FridgeManager", 1, 0, "Product");

    QQmlApplicationEngine engine;

    FridgeManager* manager = new FridgeManager();
    engine.rootContext()->setContextProperty("fridgeManager", manager);

    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));

    if (engine.rootObjects().isEmpty()) {
        qDebug() << "Failed to load QML!";
        delete manager;
        return -1;
    }

    qDebug() << "Application started successfully!";
    return app.exec();
}

#include "main.moc"