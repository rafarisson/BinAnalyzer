#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>

namespace bina::document { class Document; }

namespace bina::presentation {

// Os formatos customizados do documento (padrões nomeados, como "version_text") e o experimento
// com padrões: dá para digitar um padrão e uma amostra de valores e ver o texto que sairia.
// Acompanha o documento: abrir outra estrutura refaz a lista e o formato atual.
class FormatPatternController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList patternIds READ patternIds NOTIFY patternIdsChanged)
    Q_PROPERTY(QString currentPattern READ currentPattern WRITE setCurrentPattern NOTIFY currentPatternChanged)

    // O padrão do formato atual (vazio se não há formatos).
    Q_PROPERTY(QString currentSpec READ currentSpec NOTIFY currentSpecChanged)

public:

    explicit FormatPatternController(QObject *parent = nullptr);

    void setDocument(document::Document *document);

    QStringList patternIds() const { return m_ids; }

    QString currentPattern() const { return m_current; }
    void setCurrentPattern(const QString &id);

    QString currentSpec() const;

    // Aplica 'spec' como padrão do formato atual. O documento fica alterado e os dados analisados,
    // desatualizados. Recusas (padrão inválido) saem por errorOccurred().
    Q_INVOKABLE bool applyPattern(const QString &spec);

    // Cria um formato novo com o padrão 'spec' e o seleciona. Recusas (id repetido ou inválido,
    // padrão inválido) saem por errorOccurred().
    Q_INVOKABLE bool createPattern(const QString &id, const QString &spec);

    // Uma amostra de valores que serve de ponto de partida para o padrão.
    Q_INVOKABLE QString suggestSample(const QString &spec) const;

    // Aplica o padrão à amostra: {patternValid, ok, text, error}. Não altera o documento.
    Q_INVOKABLE QVariantMap preview(const QString &spec, const QString &sample) const;

signals:

    void patternIdsChanged();
    void currentPatternChanged();
    void currentSpecChanged();

    // O padrão foi recusado; 'message' explica o motivo.
    void errorOccurred(const QString &message);

private:

    void rebuild();

    document::Document *m_document{nullptr};
    QStringList m_ids;
    QString m_current;
    QString m_spec;
};

} // namespace bina::presentation
