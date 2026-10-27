#include <QFileInfo>

#include "bina/app/ExportService.h"
#include "bina/app/MessageLog.h"
#include "bina/app/Paths.h"

#include "bina/format/BuiltinFormatters.h"
#include "bina/model/ResultTreeModel.h"
#include "bina/model/ResultTableModel.h"

namespace bina::app {

namespace {

// O formato "string" mostra o texto entre aspas; no arquivo exportado o valor é só o texto.
QString exportedValue(const QString &formatId, QString value)
{
    if (formatId == format::FormatIds::kString && value.size() >= 2
        && value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
        return value.slice(1, value.size() - 2);

    return value;
}

QList<io::ResultNodeDto> nodesUnder(const model::ResultTreeModel &model, const QModelIndex &parent)
{
    QList<io::ResultNodeDto> nodes;

    for (int row = 0; row < model.rowCount(parent); ++row) {
        const auto text = [&](model::ResultTreeModel::Column column) {
            return model.index(row, column, parent).data(Qt::DisplayRole);
        };

        io::ResultNodeDto node;
        node.name = text(model::ResultTreeModel::NameColumn).toString();
        node.type = text(model::ResultTreeModel::TypeColumn).toString();
        node.offset = text(model::ResultTreeModel::OffsetColumn).toString();
        node.bytes = text(model::ResultTreeModel::SizeColumn).toLongLong();
        node.value = exportedValue(text(model::ResultTreeModel::FormatColumn).toString(),
                                   text(model::ResultTreeModel::ValueColumn).toString());
        node.children = nodesUnder(model, model.index(row, model::ResultTreeModel::NameColumn, parent));

        nodes.append(node);
    }

    return nodes;
}

} // namespace

ExportService::ExportService(model::ResultTreeModel &tree, model::ResultTableModel &table, MessageLog &log,
                             QObject *parent)
    : QObject{parent}
    , m_tree{tree}
    , m_table{table}
    , m_log{log}
{
}

bool ExportService::exportResult(const QString &pathOrUrl)
{
    if (m_table.rowCount() == 0) {
        m_log.add(tr("Não há resultado para exportar: analise um arquivo primeiro."), true);
        return false;
    }

    QString path;

    if (const schema::OperationResult resolved = resolveSavePath(pathOrUrl, m_codecs.defaultSuffix(), path); !resolved) {
        m_log.add(tr("Não foi possível exportar: %1").arg(resolved.error()), true);
        return false;
    }

    const io::ResultCodec *codec = m_codecs.forPath(path);

    if (!codec) {
        m_log.add(tr("Não foi possível exportar '%1': formato desconhecido '%2' (use %3).")
                      .arg(path, QFileInfo(path).suffix(), m_codecs.suffixes().join(QStringLiteral(", "))), true);
        return false;
    }

    // Os registros ainda não lidos entram no arquivo exportado.
    m_tree.fetchAll();

    const schema::OperationResult written = codec->shape == io::ResultCodec::Tree
                                                ? codec->treeExporter()->writeFile(path, resultTree(m_tree))
                                                : codec->tableExporter()->writeFile(path, resultTable(m_table));

    if (!written) {
        m_log.add(tr("Não foi possível exportar '%1': %2").arg(path, written.error()), true);
        return false;
    }

    m_log.add(tr("Resultado exportado: %1").arg(QFileInfo(path).fileName()));

    return true;
}

io::ResultTreeDto ExportService::resultTree(const model::ResultTreeModel &model)
{
    return {nodesUnder(model, {})};
}

io::ResultTableDto ExportService::resultTable(const model::ResultTableModel &model)
{
    io::ResultTableDto table;

    for (int c = 0; c < model.columnCount(); ++c)
        table.headers << model.headerData(c, Qt::Horizontal).toString();

    for (int r = 0; r < model.rowCount(); ++r) {
        QStringList row;

        for (int c = 0; c < model.columnCount(); ++c)
        {
            const QModelIndex cell = model.index(r, c);

            row << exportedValue(cell.data(model::ResultTableModel::FormatIdRole).toString(),
                                 cell.data(Qt::DisplayRole).toString());
        }

        table.rows.append(row);
    }

    return table;
}

} // namespace bina::app
