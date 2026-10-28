#pragma once

#include <QObject>
#include <QString>

#include "bina/io/Codecs.h"
#include "bina/io/ResultDto.h"

namespace bina::model { class ResultTreeModel; class ResultTableModel; }

namespace bina::app {

class MessageLog;

// Exporta o resultado da análise como está na tela. O formato vem da extensão (ResultCodecRegistry:
// .json para a árvore, .csv para a tabela; sem extensão vale o primeiro, .json). Os registros que
// ainda não foram lidos são lidos antes: o arquivo exportado tem todos. O cabeçalho, se houver, vai
// para um arquivo à parte, "<nome>.header.<extensão>", no mesmo formato.
class ExportService : public QObject
{
    Q_OBJECT

public:

    // Os models e o log precisam sobreviver ao serviço.
    ExportService(model::ResultTreeModel &tree, model::ResultTableModel &table, model::ResultTreeModel &headerTree,
                  model::ResultTableModel &headerTable, MessageLog &log, QObject *parent = nullptr);

    bool exportResult(const QString &pathOrUrl);

    const io::ResultCodecRegistry &codecs() const { return m_codecs; }

    // O resultado como a tela mostra, em texto puro, para os exportadores de bina/io. Lê dos models,
    // a fonte do que é exibido, então a exportação não refaz nenhuma formatação.
    static io::ResultTreeDto resultTree(const model::ResultTreeModel &model);
    static io::ResultTableDto resultTable(const model::ResultTableModel &model);

    // Onde o cabeçalho é gravado quando o resultado vai para 'path'.
    static QString headerPath(const QString &path);

private:

    schema::OperationResult write(const io::ResultCodec &codec, const QString &path, bool header);

    model::ResultTreeModel &m_tree;
    model::ResultTableModel &m_table;
    model::ResultTreeModel &m_headerTree;
    model::ResultTableModel &m_headerTable;
    MessageLog &m_log;
    io::ResultCodecRegistry m_codecs{io::ResultCodecRegistry::withBuiltins()};
};

} // namespace bina::app
