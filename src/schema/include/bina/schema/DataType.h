#pragma once

#include <memory>
#include <variant>

#include <QString>

#include "bina/schema/Field.h"
#include "bina/schema/Types.h"

namespace bina::schema {

// Um tipo de campo, imutável depois de criado: ou primitivo (tamanho, alinhamento e representação
// próprios) ou struct (o layout vem dos campos da definição, calculado pelo LayoutCalculator,
// porque depende do alinhamento máximo do documento).
class DataType
{
public:

    Q_DISABLE_COPY_MOVE(DataType)

    enum Category {
        Primitive,
        Struct
    };

    static std::unique_ptr<DataType> makePrimitive(const QString &id, int size, int align, Representation representation);

    // 'definition' nula é o marcador "struct", usado por campos com filhos.
    static std::unique_ptr<DataType> makeStruct(const QString &id, std::unique_ptr<Field> definition = nullptr,
                                                const QString &formatId = QString());

    QString id() const { return m_id; }
    Category category() const { return std::holds_alternative<PrimitiveInfo>(m_info) ? Primitive : Struct; }

    // Só valem para primitivos; um struct não tem tamanho próprio (0) e alinha em 1.
    int size() const;
    int align() const;
    Representation representation() const;

    QString formatId() const;
    const Field *definition() const;

private:

    struct PrimitiveInfo {
        int size;
        int align;
        Representation representation;
    };

    struct StructInfo {
        std::unique_ptr<Field> definition;
        QString formatId;
    };

    DataType(const QString &id, PrimitiveInfo info) : m_id{id}, m_info{info} {}
    DataType(const QString &id, StructInfo &&info) : m_id{id}, m_info{std::move(info)} {}

    QString m_id;
    std::variant<PrimitiveInfo, StructInfo> m_info;
};

} // namespace bina::schema
