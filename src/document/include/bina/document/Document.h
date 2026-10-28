#pragma once

#include <memory>

#include <QObject>

#include <QSysInfo>

#include "bina/format/FormatRegistry.h"
#include "bina/schema/Types.h"
#include "bina/schema/TypeRegistry.h"
#include "bina/document/FieldTree.h"

namespace bina::document {

class Document;
class TypeCatalog;
struct DocumentFieldDto;

// A fonte de tipos da árvore da estrutura: os tipos da biblioteca do documento, copiados para ele
// quando um campo passa a usá-los.
class LibraryTypeSource : public TypeSource
{
public:

    explicit LibraryTypeSource(Document &document) : m_document{document} {}

    QStringList availableTypeIds() const override;
    schema::OperationResult provideType(const QString &id) override;

private:

    Document &m_document;
};

class Document : public QObject
{
    Q_OBJECT

public:

    // Relação entre um tipo do documento e o tipo de mesmo id na biblioteca.
    enum class LibraryState {
        NotInLibrary,
        SameAsLibrary,
        DiffersFromLibrary
    };

    explicit Document(QObject *parent = nullptr);
    ~Document() override;

    int version() const { return m_version; }
    int maxAlign() const { return m_maxAlign; }

    schema::TypeRegistry *types() { return m_registry.get(); }
    const schema::TypeRegistry *types() const { return m_registry.get(); }
    format::FormatRegistry *formats() { return m_formats.get(); }
    const format::FormatRegistry *formats() const { return m_formats.get(); }
    QSysInfo::Endian byteOrder() const { return m_byteOrder; }
    void setByteOrder(QSysInfo::Endian order);

    // Alinhamento máximo dos campos: refaz os auto_pad. Recusa valores que não são potência de 2.
    bool setMaxAlign(int align);

    // Troca o padrão de um formato customizado. Sem mudança é um sucesso silencioso; se mudou,
    // avisa por formatsChanged() (os dados analisados ficam desatualizados).
    schema::OperationResult setFormatPattern(const QString &id, const QString &spec);

    // Troca a definição de um tipo customizado (os campos do tipo). Os dados analisados ficam
    // desatualizados: avisa por typesAboutToChange()/typesChanged() e, no fim, typeDefinitionChanged().
    // O motivo de uma recusa vem no OperationResult.
    schema::OperationResult setTypeDefinition(const QString &id, std::unique_ptr<schema::Field> definition);

    // Cria um tipo customizado novo, com um campo inicial (uint8) para o usuário editar. Os dados
    // analisados continuam válidos: nenhum tipo existente muda. Avisa por typesChanged() e
    // typeAdded().
    schema::OperationResult addType(const QString &id);

    // Cabeçalhos: tipos customizados marcados como cabeçalho de arquivo. Seguem as regras dos tipos
    // (definição, biblioteca, layout), mas ficam fora da lista de tipos dos campos. Pelo menos um
    // pode estar ativo: então a análise lê a struct dele no início do arquivo e os registros vêm
    // depois. Ativar ou desativar deixa os dados analisados desatualizados (activeHeaderChanged()).
    bool isHeader(const QString &id) const { return m_headerIds.contains(id); }
    QStringList headerIds() const { return m_headerIds; }
    schema::OperationResult addHeader(const QString &id);

    // Id do cabeçalho ativo, ou vazio. Recusa (false) um id que não é de um cabeçalho do documento.
    QString activeHeader() const { return m_activeHeader; }
    bool setActiveHeader(const QString &id);
    const schema::Field *activeHeaderDefinition() const;

    // Cabeçalhos da biblioteca que o documento ainda não tem.
    QStringList availableLibraryHeaderIds() const;

    // Os campos de um grupo da estrutura viram um tipo customizado novo, 'id', e o grupo passa a
    // usá-lo; o layout não muda. Avisa por typeAdded().
    schema::OperationResult extractType(schema::Field *group, const QString &id);

    // Remove um tipo customizado que nenhum campo usa (nem os da estrutura, nem os de outro tipo).
    // Avisa por typesChanged() e typeRemoved().
    schema::OperationResult removeType(const QString &id);

    // A biblioteca de tipos: precisa sobreviver ao documento (ou ser trocada antes). nullptr desliga.
    // Avisa por libraryChanged(), também quando o mesmo catálogo foi recarregado.
    void setLibrary(const TypeCatalog *library);
    const TypeCatalog *library() const { return m_library; }

    // Copia da biblioteca o tipo 'id', com os tipos e formatos de que ele depende e que o documento
    // ainda não tem; os que o documento já tem (mesmo diferentes) são mantidos. Avisa por
    // formatAdded() e typeImported() para cada item copiado.
    schema::OperationResult importType(const QString &id);

    LibraryState libraryState(const QString &id) const;

    // Troca a definição do tipo pela da biblioteca (os tipos e formatos que faltam são copiados),
    // como setTypeDefinition(). O formato padrão do tipo continua o do documento.
    schema::OperationResult updateTypeFromLibrary(const QString &id);

    // Tipos da biblioteca que o documento ainda não tem.
    QStringList availableLibraryTypeIds() const;

    // Cria um formato customizado novo com o padrão 'spec'. Avisa por formatAdded().
    schema::OperationResult addFormat(const QString &id, const QString &spec);

    FieldTree *fields() { return m_fields.get(); }
    const FieldTree *fields() const { return m_fields.get(); }

signals:

    void byteOrderChanged();
    void maxAlignChanged();
    void formatsChanged();
    void typeDefinitionChanged(const QString &id);
    void typeAdded(const QString &id);
    void typeImported(const QString &id);
    void typeRemoved(const QString &id);
    void activeHeaderChanged();
    void libraryChanged();
    void formatAdded(const QString &id);

    // O registro de tipos é substituído ao carregar uma estrutura (DocumentFactory): os ponteiros
    // para as definições dos tipos valem até typesAboutToChange().
    void typesAboutToChange();
    void typesChanged();

private:

    friend class DocumentFactory;

    // O que copiar da biblioteca, em ordem de dependência.
    struct ImportPlan {
        QStringList types;
        QStringList formats;
    };

    schema::OperationResult addEntry(const QString &id, bool header);
    schema::OperationResult planType(const QString &id, ImportPlan &plan, QStringList &visiting) const;
    schema::OperationResult planFields(const DocumentFieldDto &parent, ImportPlan &plan, QStringList &visiting) const;
    schema::OperationResult planFormat(const QString &id, ImportPlan &plan) const;
    schema::OperationResult apply(const ImportPlan &plan);

    const TypeCatalog *m_library{nullptr};
    LibraryTypeSource m_typeSource{*this};

    int m_version{1};
    int m_maxAlign{schema::Alignment::kDefaultMax};
    QSysInfo::Endian m_byteOrder{QSysInfo::LittleEndian};
    std::unique_ptr<schema::TypeRegistry> m_registry;
    std::unique_ptr<format::FormatRegistry> m_formats;
    QStringList m_headerIds;
    QString m_activeHeader;

    // Declarada por último: é destruída antes dos registros, para os quais guarda ponteiros.
    std::unique_ptr<FieldTree> m_fields;
};

} // namespace bina::document
