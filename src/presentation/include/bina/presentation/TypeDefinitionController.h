#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

#include "bina/model/SchemaTreeModel.h"

namespace bina::document { class Document; class FieldTree; }

namespace bina::presentation {

// Os tipos customizados do documento (os que têm definição própria) e a edição da definição de um
// deles por vez. Um controlador cuida dos tipos comuns (Kind::Types) e outro dos cabeçalhos de
// arquivo (Kind::Headers): as mesmas regras, listas separadas.
//
// A edição é feita em um rascunho: uma cópia da definição, em uma document::FieldTree própria, com as
// mesmas regras da estrutura (inserir, remover, mover, agrupar, auto_pad...) e mostrada pelo
// draftModel, o mesmo model da árvore da estrutura. O documento só muda em apply(); restore()
// descarta o rascunho e volta à definição do documento. Acompanha o documento: abrir outra
// estrutura refaz a lista de tipos e o rascunho.
class TypeDefinitionController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Kind kind READ kind CONSTANT)
    Q_PROPERTY(QStringList typeIds READ typeIds NOTIFY typeIdsChanged)
    Q_PROPERTY(QString currentType READ currentType WRITE setCurrentType NOTIFY currentTypeChanged)
    Q_PROPERTY(bina::model::SchemaTreeModel *draftModel READ draftModel CONSTANT)

    // O rascunho foi editado e ainda não aplicado.
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)

    // O tipo atual em relação ao de mesmo id na biblioteca de tipos.
    Q_PROPERTY(LibraryState libraryState READ libraryState NOTIFY libraryStateChanged)
    // Só cabeçalhos: o tipo atual é o cabeçalho que a análise usa (no máximo um é ativo).
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    // Tipos (ou cabeçalhos) da biblioteca que o documento ainda não tem.
    Q_PROPERTY(QStringList libraryIds READ libraryIds NOTIFY libraryIdsChanged)
    // O que o ComboBox oferece: os tipos (ou cabeçalhos) do documento e, depois deles, os da biblioteca
    // que o documento ainda não tem. Cada item é {id, text, fromLibrary}.
    Q_PROPERTY(QVariantList choices READ choices NOTIFY choicesChanged)
    // Tamanho da definição em edição, em bytes (com o alinhamento do documento).
    Q_PROPERTY(int sizeInBytes READ sizeInBytes NOTIFY sizeChanged)

public:

    enum Kind {
        Types,
        Headers
    };
    Q_ENUM(Kind)

    enum LibraryState {
        NotInLibrary,
        SameAsLibrary,
        DiffersFromLibrary
    };
    Q_ENUM(LibraryState)

    explicit TypeDefinitionController(Kind kind = Types, QObject *parent = nullptr);

    Kind kind() const { return m_kind; }

    void setDocument(document::Document *document);

    QStringList typeIds() const { return m_typeIds; }

    QString currentType() const { return m_current; }

    // Trocar de tipo descarta as edições não aplicadas.
    void setCurrentType(const QString &id);

    model::SchemaTreeModel *draftModel() { return m_draftModel; }
    document::FieldTree *draft() { return m_draft; }

    bool isModified() const { return m_modified; }

    LibraryState libraryState() const { return m_libraryState; }

    bool isActive() const;
    void setActive(bool active);
    QStringList libraryIds() const;
    QVariantList choices() const;
    int sizeInBytes() const;

    // Grava o rascunho como definição do tipo atual. Falha (com errorOccurred()) se a definição
    // for recusada, por exemplo sem campos.
    Q_INVOKABLE bool apply();

    // Cria um tipo novo (com um campo inicial) e o seleciona para edição. As edições não aplicadas
    // do tipo anterior são descartadas. Falha (com errorOccurred()) se o id for recusado.
    Q_INVOKABLE bool createType(const QString &id);

    // Seleciona um item de choices(): um do documento vira o atual; um da biblioteca é copiado para o
    // documento (importFromLibrary()) e então selecionado.
    Q_INVOKABLE bool select(const QString &id);

    // Copia da biblioteca o tipo (ou cabeçalho) 'id', com o que ele usa, e o seleciona.
    Q_INVOKABLE bool importFromLibrary(const QString &id);

    // Descarta as edições e volta à definição do documento.
    Q_INVOKABLE void restore();

    // Remove o tipo atual. Falha (com errorOccurred()) se algum campo o usa.
    Q_INVOKABLE bool removeType();

    // Troca a definição do tipo atual pela da biblioteca, descartando as edições não aplicadas.
    Q_INVOKABLE bool updateFromLibrary();

signals:

    void typeIdsChanged();
    void currentTypeChanged();
    void modifiedChanged();
    void libraryStateChanged();
    void activeChanged();
    void libraryIdsChanged();
    void choicesChanged();
    void sizeChanged();

    // A definição foi recusada; 'message' explica o motivo.
    void errorOccurred(const QString &message);

private:

    void rebuild();
    bool refreshTypeIds();
    void updateLibraryState();
    void loadDraft();
    void setModified(bool modified);

    Kind m_kind{Types};
    document::Document *m_document{nullptr};
    document::FieldTree *m_draft{nullptr};
    model::SchemaTreeModel *m_draftModel{nullptr};
    QStringList m_typeIds;
    QString m_current;
    bool m_modified{false};
    LibraryState m_libraryState{NotInLibrary};
};

} // namespace bina::presentation
