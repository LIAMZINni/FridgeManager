#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

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
        , m_databaseConnected(false)
        , m_databaseStatus("Подключение...")
    {
        initializeProducts();
    }

    QQmlListProperty<Product> products() {
        return QQmlListProperty<Product>(this, &m_products);
    }

    bool databaseConnected() const { return m_databaseConnected; }
    QString databaseStatus() const { return m_databaseStatus; }

    Q_INVOKABLE void addProductQuantity(int index, int amount) {
        if (index >= 0 && index < m_products.size()) {
            Product* product = m_products[index];
            product->setCurrentQuantity(product->currentQuantity() + amount);
            emit productsChanged();
        }
    }

    Q_INVOKABLE void removeProductQuantity(int index, int amount) {
        if (index >= 0 && index < m_products.size()) {
            Product* product = m_products[index];
            if (product->currentQuantity() >= amount) {
                product->setCurrentQuantity(product->currentQuantity() - amount);
                emit productsChanged();
            }
        }
    }

    Q_INVOKABLE QString generateOrder() {
        // Сохраняем в домашнюю директорию по умолчанию
        QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString defaultFileName = defaultPath + "/заявка_поставщику_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".txt";

        return saveOrderToFile(defaultFileName);
    }

    Q_INVOKABLE QString saveOrderToCustomLocation() {
        // В Linux используем нативный диалог через D-Bus
#ifdef Q_OS_LINUX
        QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        QString fileName = QFileDialog::getSaveFileName(nullptr,
            "Сохранить заявку",
            desktopPath + "/заявка_поставщику_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".txt",
            "Текстовые файлы (*.txt)");

        if (!fileName.isEmpty()) {
            return saveOrderToFile(fileName);
        }
        return "Отменено пользователем";
#else
        return "Функция доступна только в Linux";
#endif
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

    Q_INVOKABLE bool fileExists(const QString& filePath) {
        return QFile::exists(filePath);
    }

private:
    QString saveOrderToFile(const QString& filePath) {
        QFile file(filePath);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream.setEncoding(QStringConverter::Utf8);

            stream << "ЗАЯВКА ДЛЯ ПОСТАВЩИКА\n";
            stream << "=====================\n";
            stream << "Ресторан: 'Гурман'\n";
            stream << "Дата: " << QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm") << "\n";
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

    void initializeProducts() {
        m_products.clear();

        m_products.append(new Product(1, "Творог", 5, 10, this));
        m_products.append(new Product(2, "Сыр", 12, 15, this));
        m_products.append(new Product(3, "Молоко", 18, 20, this));
        m_products.append(new Product(4, "Яйца", 25, 30, this));
        m_products.append(new Product(5, "Оливки", 3, 8, this));

        m_databaseStatus = "Локальный режим";
        emit databaseStatusChanged();
    }

    QList<Product*> m_products;
    bool m_databaseConnected;
    QString m_databaseStatus;

signals:
    void productsChanged();
    void databaseStatusChanged();
};

int main(int argc, char* argv[])
{
    qDebug() << "Starting FridgeManager application...";

    QGuiApplication app(argc, argv);

    app.setApplicationName("FridgeManager");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Restaurant");

    qmlRegisterType<Product>("FridgeManager", 1, 0, "Product");

    QQmlApplicationEngine engine;

    FridgeManager* manager = new FridgeManager();
    engine.rootContext()->setContextProperty("fridgeManager", manager);

    qDebug() << "Loading QML...";

    // УМНАЯ ЗАГРУЗКА QML - пробуем разные пути
    QStringList possiblePaths;

    // 1. Из текущей директории (для разработки)
    possiblePaths << QDir::current().absoluteFilePath("Main.qml");

    // 2. Из системной директории (для установленного пакета)
    possiblePaths << "/usr/share/fridgemanager/Main.qml";

    // 3. Из домашней директории
    possiblePaths << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Main.qml";

    QString qmlPath;
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            qmlPath = path;
            qDebug() << "Found QML at:" << path;
            break;
        }
    }

    if (!qmlPath.isEmpty()) {
        qDebug() << "Loading QML from file:" << qmlPath;
        engine.load(QUrl::fromLocalFile(qmlPath));
    }
    else {
        qDebug() << "QML file not found in standard locations, trying resource...";
        // Последняя попытка - из ресурсов
        engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    }

    if (engine.rootObjects().isEmpty()) {
        qDebug() << "❌ FAILED TO LOAD QML!";
        qDebug() << "Checked paths:" << possiblePaths;
        delete manager;
        return -1;
    }

    qDebug() << "✅ Application started successfully!";
    return app.exec();
}

#include "main.moc"