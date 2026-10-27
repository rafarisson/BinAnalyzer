#pragma once

#include <QtQml/qqmlregistration.h>

#include "bina/model/SchemaTreeModel.h"
#include "bina/presentation/FormatPatternController.h"
#include "bina/model/ResultTreeModel.h"
#include "bina/model/ResultTableModel.h"
#include "bina/presentation/TypeDefinitionController.h"
#include "bina/session/AnalysisSession.h"

// Registro no QML dos tipos das camadas de baixo, que não dependem de Qt Quick. Cada tipo é criado
// pelo AppController; o QML só os usa.
namespace bina::app {

struct SchemaTreeModelForeign
{
    Q_GADGET
    QML_FOREIGN(bina::model::SchemaTreeModel)
    QML_NAMED_ELEMENT(SchemaTreeModel)
    QML_UNCREATABLE("Criado pelo AppController")
};

struct ResultTreeModelForeign
{
    Q_GADGET
    QML_FOREIGN(bina::model::ResultTreeModel)
    QML_NAMED_ELEMENT(ResultTreeModel)
    QML_UNCREATABLE("Criado pelo AppController")
};

struct ResultTableModelForeign
{
    Q_GADGET
    QML_FOREIGN(bina::model::ResultTableModel)
    QML_NAMED_ELEMENT(ResultTableModel)
    QML_UNCREATABLE("Criado pelo AppController")
};

struct TypeDefinitionControllerForeign
{
    Q_GADGET
    QML_FOREIGN(bina::presentation::TypeDefinitionController)
    QML_NAMED_ELEMENT(TypeDefinitionController)
    QML_UNCREATABLE("Criado pelo AppController")
};

struct FormatPatternControllerForeign
{
    Q_GADGET
    QML_FOREIGN(bina::presentation::FormatPatternController)
    QML_NAMED_ELEMENT(FormatPatternController)
    QML_UNCREATABLE("Criado pelo AppController")
};

struct AnalysisSessionForeign
{
    Q_GADGET
    QML_FOREIGN(bina::session::AnalysisSession)
    QML_NAMED_ELEMENT(AnalysisSession)
    QML_UNCREATABLE("Criado pelo AppController")
};

} // namespace bina::app
