#pragma once

#include <memory>

#include <QObject>

#include <QSysInfo>

#include "bina/format/FormatRegistry.h"
#include "bina/schema/Types.h"
#include "bina/schema/TypeRegistry.h"
#include "bina/document/FieldTree.h"

namespace bina::document {

class Document : public QObject
{
    Q_OBJECT

public:

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
    void formatAdded(const QString &id);

    // O registro de tipos é substituído ao carregar uma estrutura (DocumentFactory): os ponteiros
    // para as definições dos tipos valem até typesAboutToChange().
    void typesAboutToChange();
    void typesChanged();

private:

    friend class DocumentFactory;

    int m_version{1};
    int m_maxAlign{schema::Alignment::kDefaultMax};
    QSysInfo::Endian m_byteOrder{QSysInfo::LittleEndian};
    std::unique_ptr<schema::TypeRegistry> m_registry;
    std::unique_ptr<format::FormatRegistry> m_formats;

    // Declarada por último: é destruída antes dos registros, para os quais guarda ponteiros.
    std::unique_ptr<FieldTree> m_fields;
};

} // namespace bina::document
