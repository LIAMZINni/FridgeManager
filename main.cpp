#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QLocale>
#include <QTextCodec>

#include "DatabaseManager.h"
#include "ProtobufSerializer.h"

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

    // Конвертация в ProductData для protobuf
    ProductData toProductData() const {
        return ProductData(m_id, m_name, m_currentQuantity, m_normQuantity);
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
        , m_protobufSerializer(new ProtobufSerializer(this))
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
                if (m_dbManager.addProductQuantity(product->id(), amount)) {
                    product->setCurrentQuantity(product->currentQuantity() + amount);
                }
            }
            else {
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
                    if (m_dbManager.removeProductQuantity(product->id(), amount)) {
                        product->setCurrentQuantity(product->currentQuantity() - amount);
                    }
                }
                else {
                    product->setCurrentQuantity(product->currentQuantity() - amount);
                }
                emit productsChanged();
            }
        }
    }

    // Основные методы сохранения
    Q_INVOKABLE QString generateOrder() {
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString baseName = defaultPath + "/order_supplier_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");

        // Сохраняем в оба формата
        QString txtResult = saveOrderToFile(baseName + ".txt");
        QString protoResult = saveOrderToProtobuf(baseName + ".bin");

        if (txtResult.startsWith("Success:")) {
            m_lastSavePath = baseName + ".txt";
            emit lastSavePathChanged();
        }

        return txtResult + "\n" + protoResult;
    }

    Q_INVOKABLE QString saveOrderToPath(const QString& directoryPath) {
        QString baseName = directoryPath + "/order_supplier_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");

        QString txtResult = saveOrderToFile(baseName + ".txt");
        QString protoResult = saveOrderToProtobuf(baseName + ".bin");

        if (txtResult.startsWith("Success:")) {
            m_lastSavePath = baseName + ".txt";
            emit lastSavePathChanged();
        }

        return txtResult + "\n" + protoResult;
    }

    // НОВЫЕ МЕТОДЫ ДЛЯ PROTOBUF
    Q_INVOKABLE QString exportToProtobuf(const QString& directoryPath) {
        QString filePath = directoryPath + "/products_backup_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".bin";
        
        QVector<ProductData> productsData;
        for (Product* product : m_products) {
            productsData.append(product->toProductData());
        }

        if (m_protobufSerializer->exportProducts(productsData, filePath)) {
            return "Success: Products exported to " + filePath;
        } else {
            return "Error: " + m_protobufSerializer->getLastError();
        }
    }

    Q_INVOKABLE QString importFromProtobuf(const QString& filePath) {
        QVector<ProductData> productsData = m_protobufSerializer->importProducts(filePath);
        
        if (productsData.isEmpty()) {
            return "Error: " + m_protobufSerializer->getLastError();
        }

        // Обновляем список продуктов
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

        emit productsChanged();
        return "Success: Loaded " + QString::number(productsData.size()) + " products from " + filePath;
    }

    Q_INVOKABLE QString saveOrderProtobufOnly(const QString& directoryPath) {
        QString filePath = directoryPath + "/order_protobuf_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".bin";
        
        QVector<ProductData> productsToOrder;
        for (Product* product : m_products) {
            if (product->needsOrder()) {
                productsToOrder.append(product->toProductData());
            }
        }

        if (m_protobufSerializer->exportOrder(productsToOrder, filePath, "Gourmet")) {
            return "Success: Order saved in protobuf: " + filePath;
        } else {
            return "Error: " + m_protobufSerializer->getLastError();
        }
    }

    Q_INVOKABLE QString loadOrderFromProtobuf(const QString& filePath) {
        QVector<ProductData> productsData = m_protobufSerializer->importProducts(filePath);
        
        if (productsData.isEmpty()) {
            return "Error: " + m_protobufSerializer->getLastError();
        }

        QString result = "📊 Order loaded from Protobuf:\n\n";
        result += "File: " + filePath + "\n";
        result += "Products count: " + QString::number(productsData.size()) + "\n\n";
        
        int totalOrder = 0;
        for (const auto& product : productsData) {
            int orderQty = qMax(0, product.normQuantity - product.currentQuantity);
            if (orderQty > 0) {
                result += "• " + product.name + ": " + QString::number(orderQty) + " packs\n";
                totalOrder += orderQty;
            }
        }
        
        if (totalOrder > 0) {
            result += "\nTotal to order: " + QString::number(totalOrder) + " packs";
        } else {
            result += "\nNo products to order";
        }
        
        return result;
    }

    // Существующие методы без изменений
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
        dirs << getDefaultHomePath() + "/Orders";
        dirs << getDesktopPath();
        dirs << getDefaultDocumentsPath();
        dirs << getDefaultDownloadsPath();
        dirs << QDir::currentPath() + "/orders";

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
        qDebug() << "🔄 Initializing database connection...";

        if (m_dbManager.connectToDatabase() && m_dbManager.isConnected()) {
            m_databaseConnected = true;
            m_databaseStatus = "✅ PostgreSQL database connected";

            auto productsData = m_dbManager.getAllProducts();
            if (!productsData.isEmpty()) {
                loadProductsFromDatabase(productsData);
                qDebug() << "✅ Loaded products from DB:" << productsData.size();
            }
            else {
                m_databaseStatus = "❌ DB connected but no products found";
                initializeLocalProducts();
            }
        }
        else {
            m_databaseConnected = false;
            m_databaseStatus = "📋 Local mode (DB unavailable)";
            qDebug() << "❌ PostgreSQL unavailable:" << m_dbManager.getLastError();
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
        m_products.append(new Product(1, "Cottage cheese", 5, 10, this));
        m_products.append(new Product(2, "Cheese", 12, 15, this));
        m_products.append(new Product(3, "Milk", 18, 20, this));
        m_products.append(new Product(4, "Eggs", 25, 30, this));
        m_products.append(new Product(5, "Olives", 3, 8, this));
        qDebug() << "📋 Using local test data";
    }

    QString saveOrderToFile(const QString& filePath) {
        QFileInfo fileInfo(filePath);
        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                return "Error: Cannot create directory " + dir.absolutePath();
            }
        }

        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            
            // Устанавливаем UTF-8 кодировку
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            stream.setCodec("UTF-8");
#else
            stream.setEncoding(QStringConverter::Utf8);
#endif
            stream.setGenerateByteOrderMark(false);

            stream << "=========================================\n";
            stream << "           SUPPLIER ORDER\n";
            stream << "=========================================\n";
            stream << "Restaurant: 'Gourmet'\n";
            stream << "Date: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") << "\n";
            stream << "DB Status: " << m_databaseStatus << "\n";
            stream << "=========================================\n\n";

            bool hasOrders = false;
            int totalPacks = 0;

            stream << "PRODUCTS TO ORDER:\n";
            stream << "-----------------------------------------\n";

            for (Product* product : m_products) {
                if (product->needsOrder()) {
                    int orderQty = product->orderQuantity();
                    stream << "- " << product->name() << ": " << orderQty << " packs\n";
                    hasOrders = true;
                    totalPacks += orderQty;
                }
            }

            if (!hasOrders) {
                stream << "All products are in sufficient quantity.\n";
            }
            else {
                stream << "\n-----------------------------------------\n";
                stream << "TOTAL TO ORDER: " << totalPacks << " packs\n";
            }

            // Current stock
            stream << "\n=========================================\n";
            stream << "           CURRENT STOCK\n";
            stream << "=========================================\n";

            for (Product* product : m_products) {
                stream << "- " << product->name() << ": " << product->currentQuantity()
                    << " / " << product->normQuantity() << " packs";
                if (product->needsOrder()) {
                    stream << " (NEED " << product->orderQuantity() << ")";
                }
                stream << "\n";
            }

            file.close();

            if (QFile::exists(filePath) && QFileInfo(filePath).size() > 0) {
                qDebug() << "File saved:" << filePath;
                return "Success: Order saved to " + filePath;
            }
            else {
                return "Error: File created but empty";
            }
        }
        else {
            return "Error: Cannot create file " + filePath;
        }
    }

    QString saveOrderToProtobuf(const QString& filePath) {
        QVector<ProductData> productsToOrder;
        for (Product* product : m_products) {
            if (product->needsOrder()) {
                productsToOrder.append(product->toProductData());
            }
        }

        if (m_protobufSerializer->exportOrder(productsToOrder, filePath, "Gourmet")) {
            return "Success: Protobuf order saved to " + filePath;
        } else {
            return "Error Protobuf: " + m_protobufSerializer->getLastError();
        }
    }

    QList<Product*> m_products;
    DatabaseManager m_dbManager;
    ProtobufSerializer* m_protobufSerializer;
    bool m_databaseConnected;
    QString m_databaseStatus;
    QString m_lastSavePath;
};

bool loadQml(QQmlApplicationEngine& engine) {
    QStringList possiblePaths;

    possiblePaths << QDir::current().absoluteFilePath("Main.qml");
    possiblePaths << QDir::current().absoluteFilePath("qml/Main.qml");
    possiblePaths << "/usr/share/fridgemanager/Main.qml";
    possiblePaths << "/usr/local/share/fridgemanager/Main.qml";
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    possiblePaths << homePath + "/.local/share/FridgeManager/Main.qml";
    possiblePaths << "qrc:/Main.qml";

    for (const QString& path : possiblePaths) {
        qDebug() << "Checking QML path:" << path;

        if (path.startsWith("qrc:")) {
            engine.load(QUrl(path));
        }
        else if (QFile::exists(path)) {
            qDebug() << "Loading from filesystem:" << path;
            engine.load(QUrl::fromLocalFile(path));
        }
        else {
            continue;
        }

        if (!engine.rootObjects().isEmpty()) {
            qDebug() << "✅ Successfully loaded QML from:" << path;
            return true;
        }
    }

    return false;
}

int main(int argc, char* argv[])
{
    // Устанавливаем локаль и кодировку для Linux
    setlocale(LC_ALL, "C.UTF-8");
    qputenv("LC_ALL", "C.UTF-8");
    qputenv("LANG", "C.UTF-8");
    
    qDebug() << "Starting FridgeManager application...";

    QGuiApplication app(argc, argv);

    // Устанавливаем кодировку по умолчанию
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    app.setApplicationName("FridgeManager");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Restaurant");

    qmlRegisterType<Product>("FridgeManager", 1, 0, "Product");

    QQmlApplicationEngine engine;

    FridgeManager* manager = new FridgeManager();
    engine.rootContext()->setContextProperty("fridgeManager", manager);

    qDebug() << "Loading QML...";

    if (!loadQml(engine)) {
        qDebug() << "❌ FAILED TO LOAD QML!";
        qDebug() << "Please ensure Main.qml exists in one of the standard locations";
        delete manager;
        return -1;
    }

    qDebug() << "✅ Application started successfully!";
    return app.exec();
}

#include "main.moc"
