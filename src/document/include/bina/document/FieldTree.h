#pragma once

#include <QObject>

#include "bina/schema/Field.h"

namespace bina::format { class FormatRegistry; }
namespace bina::schema { class TypeRegistry; }

namespace bina::document {

// A árvore de campos da estrutura. Concentra as alterações (com sinais para as views) e mantém
// os auto_pad coerentes com o alinhamento.
//
// A implementação está dividida por assunto:
//   FieldTree.cpp         consultas, troca da raiz e alteração de um campo (nome, tipo, count, formato)
//   FieldTreeEditing.cpp  inserir, remover, mover, soltar e agrupar campos
//   FieldTreePadding.cpp  aplicação dos auto_pad (a regra está em PaddingSynchronizer)
class FieldTree : public QObject
{
    Q_OBJECT

public:

    explicit FieldTree(QObject *parent = nullptr);

    schema::Field *root() const { return m_root.get(); }
    void setRoot(std::unique_ptr<schema::Field> field);

    // Necessário para validar tipos e calcular tamanhos e auto_pad.
    void setRegistry(const schema::TypeRegistry *registry) { m_registry = registry; }

    // Alinhamento máximo dos campos (do documento), usado nos tamanhos e nos auto_pad. Quem o altera
    // chama resyncPadding() em seguida.
    void setMaxAlign(int align) { m_maxAlign = align; }

    // Quando esta árvore é a definição de um tipo, esse tipo (e os que dependem dele) não pode ser
    // usado pelos campos: a definição conteria a si mesma.
    void setEditedType(const QString &id) { m_editedType = id; }

    // Opcional: quando definido, setFormatId() só aceita formatos compatíveis com o campo.
    void setFormatRegistry(const format::FormatRegistry *formats) { m_formats = formats; }

    // Muda a cada alteração da lista de campos (inserir, remover, mover, trocar a raiz e auto_pad).
    // Permite detectar referências por posição que ficaram desatualizadas.
    quint32 revision() const { return m_revision; }

    // Tipos que um campo pode usar (primitivos e customizados); "struct" é implícito.
    QStringList typeIds() const;

    // Formatos compatíveis com o tipo e o count do campo.
    QStringList compatibleFormatIds(const schema::Field *field) const;

    // Tamanho total do campo (inclui os filhos, count e auto_pad); 0 se inválido.
    int sizeInBytes(const schema::Field *field) const;

    // Alterações de um campo. Retornam false se nada mudou ou se foram recusadas; as recusas
    // que o usuário precisa entender também são anunciadas por editRejected().
    bool setName(schema::Field *field, const QString &name);
    bool setTypeId(schema::Field *field, const QString &id);
    bool setCount(schema::Field *field, int count);
    bool setFormatId(schema::Field *field, const QString &id);

    // Edição da lista de campos. Um auto_pad nunca participa: não é inserido, removido nem
    // movido, e os campos que ficam sem filhos ou ganham filhos têm o tipo e os auto_pad ajustados.
    bool insert(schema::Field *parent, int position, int count);
    bool remove(schema::Field *parent, int position, int count);
    bool move(schema::Field *srcParent, int srcRow, schema::Field *dstParent, int dstRow);

    // Insere um campo novo na posição 'row' de 'parent' (limitada ao auto_pad final).
    schema::Field *insertField(schema::Field *parent, int row);

    schema::Field *addAfter(schema::Field *field);           // irmão logo abaixo
    schema::Field *addAfterParent(schema::Field *field);     // irmão do pai, logo abaixo dele
    schema::Field *addInside(schema::Field *field);          // filho no fim (o campo vira struct)
    bool removeField(schema::Field *field);                  // o campo e seus filhos

    enum DropZone {
        Before,     // como irmão, antes do alvo
        Onto,       // dentro do alvo; um alvo sem filhos e o campo movido formam uma nova struct
        After       // como irmão, depois do alvo
    };

    bool canDrop(const schema::Field *source, const schema::Field *target, DropZone zone) const;
    bool drop(schema::Field *source, schema::Field *target, DropZone zone);

    // Recalcula todos os auto_pad e avisa as views de que os tamanhos mudaram. Necessário quando algo
    // fora da árvore altera o layout (por exemplo, o alinhamento máximo do documento).
    void resyncPadding();

signals:

    void aboutToReset();
    void reset();
    void aboutToInsert(const schema::Field *parent, int first, int last);
    void inserted();
    void aboutToRemove(const schema::Field *parent, int first, int last);
    void removed();
    void aboutToMove(const schema::Field *srcParent, int srcRow, const schema::Field *dstParent, int dstRow);
    void moved();
    void fieldChanged(const schema::Field *node);

    // Uma alteração pedida pelo usuário foi recusada; 'message' explica o motivo.
    void editRejected(const QString &message);

private:

    // Auto_pad
    void updatePadding(schema::Field *from);
    void syncPadding(schema::Field *container);
    void normalize(schema::Field *container);
    void emitChangedRecursively(const schema::Field *field);

    // Edição
    bool groupWith(schema::Field *target, schema::Field *source);
    QString uniqueFieldName(const schema::Field *parent, const QString &base) const;
    int calculateInsertPosition(const schema::Field *parent) const;
    int calculateDestinationRowForMove(const schema::Field *srcParent, int srcRow, schema::Field *dstParent, int dstRow) const;

    // Núcleo
    bool usesEditedType(const QString &typeId) const;
    bool hasSiblingNamed(const schema::Field *field, const QString &name) const;
    void dropIncompatibleFormat(schema::Field *field);
    void becomeStruct(schema::Field *parent);
    void reject(const QString &message);
    void touch() { ++m_revision; }

    quint32 m_revision{1};
    const schema::TypeRegistry *m_registry{nullptr};
    int m_maxAlign{4};
    QString m_editedType;
    const format::FormatRegistry *m_formats{nullptr};
    std::unique_ptr<schema::Field> m_root;
};

} // namespace bina::document
