#pragma once

#include <memory>
#include <vector>

#include <QList>
#include <QString>

namespace bina::schema {

class Field
{
public:

    Q_DISABLE_COPY_MOVE(Field)

    explicit Field(const QString &name = QString());
    ~Field();

    // Campo de preenchimento (uint8 x size) criado pelo FieldTree. É identificado por esta marca,
    // não pelo nome: o nome "auto_pad" é só o rótulo de exibição e um campo comum pode usá-lo.
    static std::unique_ptr<Field> makeAutoPad(int size);
    bool isAutoPad() const { return m_autoPad; }

    // Identidade estável do campo durante a execução: não muda com edições nem ao mover o campo,
    // e nunca é reaproveitada (uma cópia por clone() recebe outra). Serve para referências que
    // sobrevivem à árvore, como as escolhas de formato dos dados analisados ou o arrastar e soltar.
    using Id = quint64;
    Id id() const { return m_id; }

    // Resultado dos setters: distingue um valor recusado de um valor que já era o atual.
    enum class Edit {
        Changed,
        Unchanged,
        Invalid
    };

    QString name() const { return m_name; }
    Edit setName(const QString &name);

    QString typeId() const { return m_typeId; }
    Edit setTypeId(const QString &id);

    // Um campo com filhos é sempre uma struct, qualquer que seja o typeId guardado: o layout, a
    // análise e a formatação usam os filhos.
    bool isStruct() const;

    // Quantidade de elementos: de 1 a kMaxCount.
    static constexpr int kMaxCount = 1000000;

    int count() const { return m_count; }
    Edit setCount(int count);

    QString formatId() const { return m_formatId; }
    Edit setFormatId(const QString &id);

    Field *parent() const { return m_parent; }
    int row() const;
    int childCount() const { return static_cast<int>(m_children.size()); }
    Field *child(int row) const;

    // Cópia profunda. A definição de um tipo não guarda auto_pad: 'withAutoPads' falso os deixa de fora.
    std::unique_ptr<Field> clone(bool withAutoPads = true) const;

    Field *insertChild(int position, const QString &name);
    // Só consome 'field' se a inserção for aceita; em caso de falha o chamador continua dono.
    bool insertExistingChild(int position, std::unique_ptr<Field> &&field);
    std::unique_ptr<Field> takeChild(int position);
    bool removeChild(int position);

private:

    Id m_id;
    QString m_name;
    QString m_typeId;
    int m_count{1};
    QString m_formatId;
    bool m_autoPad{false};
    Field *m_parent{nullptr};
    std::vector<std::unique_ptr<Field>> m_children;
};

} // namespace bina::schema
