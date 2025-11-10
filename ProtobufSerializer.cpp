#include "ProtobufSerializer.h"
#include <QFile>
#include <QDebug>
#include <QDateTime>

ProtobufSerializer::ProtobufSerializer(QObject* parent)
    : QObject(parent)
{
}

QByteArray ProtobufSerializer::serializeProducts(const QVector<ProductData>& products)
{
    try {
        fridgemanager::ProductListProto productList;

        // Устанавливаем метаданные
        productList.set_timestamp(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString());
        productList.set_version("1.0");

        // Добавляем продукты
        for (const auto& product : products) {
            auto* protoProduct = productList.add_products();
            protoProduct->set_id(product.id);
            protoProduct->set_name(product.name.toStdString());
            protoProduct->set_current_quantity(product.currentQuantity);
            protoProduct->set_norm_quantity(product.normQuantity);
        }

        // Сериализуем в строку
        std::string serialized = productList.SerializeAsString();
        return QByteArray(serialized.data(), serialized.size());

    }
    catch (const std::exception& e) {
        m_lastError = QString("Serialization error: %1").arg(e.what());
        qWarning() << m_lastError;
        return QByteArray();
    }
}

QVector<ProductData> ProtobufSerializer::deserializeProducts(const QByteArray& data)
{
    QVector<ProductData> products;

    try {
        fridgemanager::ProductListProto productList;

        if (!productList.ParseFromArray(data.constData(), data.size())) {
            m_lastError = "Failed to parse protobuf data";
            qWarning() << m_lastError;
            return products;
        }

        // Извлекаем продукты
        for (int i = 0; i < productList.products_size(); ++i) {
            const auto& protoProduct = productList.products(i);
            products.append(ProductData(
                protoProduct.id(),
                QString::fromStdString(protoProduct.name()),
                protoProduct.current_quantity(),
                protoProduct.norm_quantity()
            ));
        }

        qDebug() << "Deserialized" << products.size() << "products from protobuf";

    }
    catch (const std::exception& e) {
        m_lastError = QString("Deserialization error: %1").arg(e.what());
        qWarning() << m_lastError;
    }

    return products;
}

QByteArray ProtobufSerializer::serializeOrder(const QVector<ProductData>& productsToOrder,
    const QString& restaurantName)
{
    try {
        fridgemanager::OrderProto order;

        // Устанавливаем метаданные заявки
        order.set_order_date(QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm").toStdString());
        order.set_restaurant_name(restaurantName.toStdString());

        // Добавляем продукты для заказа
        int totalPacks = 0;
        for (const auto& product : productsToOrder) {
            if (product.currentQuantity < product.normQuantity) {
                auto* protoProduct = order.add_products_to_order();
                protoProduct->set_id(product.id);
                protoProduct->set_name(product.name.toStdString());
                protoProduct->set_current_quantity(product.currentQuantity);
                protoProduct->set_norm_quantity(product.normQuantity);

                totalPacks += (product.normQuantity - product.currentQuantity);
            }
        }

        order.set_total_packs(totalPacks);

        // Сериализуем
        std::string serialized = order.SerializeAsString();
        return QByteArray(serialized.data(), serialized.size());

    }
    catch (const std::exception& e) {
        m_lastError = QString("Order serialization error: %1").arg(e.what());
        qWarning() << m_lastError;
        return QByteArray();
    }
}

bool ProtobufSerializer::saveToFile(const QByteArray& data, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        m_lastError = QString("Cannot open file for writing: %1").arg(filePath);
        return false;
    }

    qint64 bytesWritten = file.write(data);
    file.close();

    if (bytesWritten != data.size()) {
        m_lastError = "Failed to write all data to file";
        return false;
    }

    qDebug() << "Saved" << bytesWritten << "bytes to" << filePath;
    return true;
}

QByteArray ProtobufSerializer::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QString("Cannot open file for reading: %1").arg(filePath);
        return QByteArray();
    }

    QByteArray data = file.readAll();
    file.close();

    qDebug() << "Loaded" << data.size() << "bytes from" << filePath;
    return data;
}

bool ProtobufSerializer::exportProducts(const QVector<ProductData>& products, const QString& filePath)
{
    QByteArray data = serializeProducts(products);
    if (data.isEmpty()) {
        return false;
    }

    return saveToFile(data, filePath);
}

QVector<ProductData> ProtobufSerializer::importProducts(const QString& filePath)
{
    QByteArray data = loadFromFile(filePath);
    if (data.isEmpty()) {
        return QVector<ProductData>();
    }

    return deserializeProducts(data);
}

bool ProtobufSerializer::exportOrder(const QVector<ProductData>& productsToOrder,
    const QString& filePath,
    const QString& restaurantName)
{
    QByteArray data = serializeOrder(productsToOrder, restaurantName);
    if (data.isEmpty()) {
        return false;
    }

    return saveToFile(data, filePath);
}

fridgemanager::ProductProto ProtobufSerializer::productToProto(const ProductData& product)
{
    fridgemanager::ProductProto proto;
    proto.set_id(product.id);
    proto.set_name(product.name.toStdString());
    proto.set_current_quantity(product.currentQuantity);
    proto.set_norm_quantity(product.normQuantity);
    return proto;
}

ProductData ProtobufSerializer::protoToProduct(const fridgemanager::ProductProto& proto)
{
    return ProductData(
        proto.id(),
        QString::fromStdString(proto.name()),
        proto.current_quantity(),
        proto.norm_quantity()
    );
}

QString ProtobufSerializer::getLastError() const
{
    return m_lastError;
}