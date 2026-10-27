#pragma once

#include <memory>

#include <QHash>
#include <QString>
#include <QSet>
#include <QStringList>

#include "bina/schema/Types.h"
#include "bina/schema/OperationResult.h"

namespace bina::schema {

class Field;
class DataType;

// Os tipos de um documento. Não é um QObject: quem precisa avisar sobre mudanças é o Document, que é
// o único que altera o registro.
class TypeRegistry
{
public:

    Q_DISABLE_COPY_MOVE(TypeRegistry)

    // Registro vazio. Para os tipos primitivos e o marcador "struct", use withBuiltins().
    TypeRegistry();
    ~TypeRegistry();

    static std::unique_ptr<TypeRegistry> withBuiltins();

    QList<const DataType *> types() const;
    // A definição de um tipo struct customizado (nullptr nos demais). É só leitura: uma definição só
    // muda por replaceDefinition(), que valida a nova.
    const Field *definition(const QString &id) const;

    const DataType *find(const QString &id) const { return m_types.value(id); }
    bool contains(const QString &id) const { return m_types.contains(id); }

    OperationResult add(const QString &id, int size, int align, Representation representation = Representation::Opaque);

    // Os campos da definição só podem usar tipos já registrados, o que impede ciclos.
    OperationResult add(const QString &id, std::unique_ptr<Field> definition, const QString &formatId = QString());

    // Um tipo cujos campos usam 'target' (ele mesmo incluído), direta ou indiretamente.
    bool dependsOn(const QString &typeId, const QString &target) const;

    // Troca a definição de um tipo struct customizado (o id e o formato padrão continuam). A nova
    // definição segue as regras de add() e não pode conter o próprio tipo, nem por outro tipo.
    // checkReplacement() só valida; replaceDefinition() valida de novo e troca.
    OperationResult checkReplacement(const QString &id, const Field *definition) const;
    OperationResult replaceDefinition(const QString &id, std::unique_ptr<Field> definition);

    // Marcador "struct": o tipo implícito de campos com filhos, sem definição própria.
    OperationResult addStructMarker(const QString &id);

private:

    OperationResult checkNewId(const QString &id, QString &normalized) const;
    OperationResult validateDefinition(const QString &id, const Field *definition) const;
    // 'checked' guarda os tipos já visitados nesta consulta que não usam 'target': cada tipo é
    // examinado uma vez, mesmo quando é usado por muitos campos.
    bool dependsOn(const QString &typeId, const QString &target, QSet<QString> &checked) const;
    bool usesType(const Field *field, const QString &target, QSet<QString> &checked) const;
    bool usesType(const Field *field, const QString &target) const;
    void insert(std::unique_ptr<DataType> type);

    QHash<QString, DataType *> m_types;
    QStringList m_order;
};

} // namespace bina::schema
