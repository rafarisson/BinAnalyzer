#pragma once

#include <QObject>
#include <QStringList>

#include "bina/model/SchemaTreeModel.h"

namespace bina::document { class Document; class FieldTree; }

namespace bina::presentation {

// Os tipos customizados do documento (os que têm definição própria) e a edição da definição de um
// deles por vez.
//
// A edição é feita em um rascunho: uma cópia da definição, em uma document::FieldTree própria, com as
// mesmas regras da estrutura (inserir, remover, mover, agrupar, auto_pad...) e mostrada pelo
// draftModel, o mesmo model da árvore da estrutura. O documento só muda em apply(); restore()
// descarta o rascunho e volta à definição do documento. Acompanha o documento: abrir outra
// estrutura refaz a lista de tipos e o rascunho.
class TypeDefinitionController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList typeIds READ typeIds NOTIFY typeIdsChanged)
    Q_PROPERTY(QString currentType READ currentType WRITE setCurrentType NOTIFY currentTypeChanged)
    Q_PROPERTY(bina::model::SchemaTreeModel *draftModel READ draftModel CONSTANT)

    // O rascunho foi editado e ainda não aplicado.
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)

    // O tipo atual em relação ao de mesmo id na biblioteca de tipos.
    Q_PROPERTY(LibraryState libraryState READ libraryState NOTIFY libraryStateChanged)

public:

    enum LibraryState {
        NotInLibrary,
        SameAsLibrary,
        DiffersFromLibrary
    };
    Q_ENUM(LibraryState)

    explicit TypeDefinitionController(QObject *parent = nullptr);

    void setDocument(document::Document *document);

    QStringList typeIds() const { return m_typeIds; }

    QString currentType() const { return m_current; }

    // Trocar de tipo descarta as edições não aplicadas.
    void setCurrentType(const QString &id);

    model::SchemaTreeModel *draftModel() { return m_draftModel; }
    document::FieldTree *draft() { return m_draft; }

    bool isModified() const { return m_modified; }

    LibraryState libraryState() const { return m_libraryState; }

    // Grava o rascunho como definição do tipo atual. Falha (com errorOccurred()) se a definição
    // for recusada, por exemplo sem campos.
    Q_INVOKABLE bool apply();

    // Cria um tipo novo (com um campo inicial) e o seleciona para edição. As edições não aplicadas
    // do tipo anterior são descartadas. Falha (com errorOccurred()) se o id for recusado.
    Q_INVOKABLE bool createType(const QString &id);

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

    // A definição foi recusada; 'message' explica o motivo.
    void errorOccurred(const QString &message);

private:

    void rebuild();
    bool refreshTypeIds();
    void updateLibraryState();
    void loadDraft();
    void setModified(bool modified);

    document::Document *m_document{nullptr};
    document::FieldTree *m_draft{nullptr};
    model::SchemaTreeModel *m_draftModel{nullptr};
    QStringList m_typeIds;
    QString m_current;
    bool m_modified{false};
    LibraryState m_libraryState{NotInLibrary};
};

} // namespace bina::presentation
