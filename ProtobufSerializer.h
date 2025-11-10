#ifndef PROTOBUFSERIALIZER_H
#define PROTOBUFSERIALIZER_H

#include <QObject>
#include <QString>
#include <QVector>
#include "product.pb.h"

struct ProductData {
    int id;
    QString name;
    int currentQuantity;
    int normQuantity;

    ProductData(int i = 0, const QString& n = "", int curr = 0, int norm = 0)
        : id(i), name(n), currentQuantity(curr), normQuantity(norm) {
    }
};

class ProtobufSerializer : public QObject
{
    Q_OBJECT

public:
    explicit ProtobufSerializer(QObject* parent = nullptr);

    // Сериализация списка продуктов в protobuf
    QByteArray serializeProducts(const QVector<ProductData>& products);

    // Десериализация списка продуктов из protobuf
    QVector<ProductData> deserializeProducts(const QByteArray& data);

    // Сериализация заявки в protobuf
    QByteArray serializeOrder(const QVector<ProductData>& productsToOrder,
        const QString& restaurantName = "Гурман");

    // Сохранение protobuf в файл
    bool saveToFile(const QByteArray& data, const QString& filePath);

    // Загрузка protobuf из файла
    QByteArray loadFromFile(const QString& filePath);

    // Экспорт продуктов в protobuf файл
    bool exportProducts(const QVector<ProductData>& products, const QString& filePath);

    // Импорт продуктов из protobuf файла
    QVector<ProductData> importProducts(const QString& filePath);

    // Экспорт заявки в protobuf файл
    bool exportOrder(const QVector<ProductData>& productsToOrder,
        const QString& filePath,
        const QString& restaurantName = "Гурман");

    QString getLastError() const;

private:
    QString m_lastError;

    // Конвертация между нашими структурами и protobuf
    fridgemanager::ProductProto productToProto(const ProductData& product);
    ProductData protoToProduct(const fridgemanager::ProductProto& proto);
};

#endif