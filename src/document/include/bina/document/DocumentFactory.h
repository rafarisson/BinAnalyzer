#pragma once

#include <QString>
#include <QStringList>

#include "bina/document/Document.h"
#include "bina/document/DocumentDto.h"
#include "bina/schema/OperationResult.h"

namespace bina::format { class FormatRegistry; }
namespace bina::schema { class Field; }

namespace bina::document {

// Converte entre o DTO (usado pela camada IO) e os objetos do Document.
class DocumentFactory
{
public:

    // Atômico: em caso de falha, 'out' não é alterado e o resultado diz o motivo. Em caso de
    // sucesso, 'out' avisa as mudanças como em uma edição (typesAboutToChange()/typesChanged() e,
    // se mudaram, byteOrderChanged() e maxAlignChanged()).
    schema::OperationResult create(const DocumentDto &data, Document &out);

    static DocumentDto toDto(const Document &doc);

    // Conversão de um campo (e seus filhos). toFieldDto() deixa os auto_pad de fora.
    static std::unique_ptr<schema::Field> buildField(const DocumentFieldDto &data);
    static DocumentFieldDto toFieldDto(const schema::Field &field);

private:

    bool validateNames(const DocumentFieldDto &parent);
    bool validateFormats(const schema::Field &field, const schema::TypeRegistry &types, const format::FormatRegistry &formats);

    QString m_error;
};

} // namespace bina::document
