#pragma once

#include <QString>

#include "bina/schema/OperationResult.h"

namespace bina::app {

// A pasta dos dados do programa para a pessoa: preferências (settings.ini) e a biblioteca de tipos
// e formatos (types/). Fica no mesmo lugar em todas as máquinas do usuário, sem pasta de organização
// (no Windows, %LOCALAPPDATA%\BinAnalyzer). O nome da aplicação precisa estar definido.
QString appDataDirectory();

// Caminho local a partir de um caminho ou de uma URL "file:" (como entregue pelos diálogos do QML).
QString localPath(const QString &pathOrUrl);

// Um caminho de recurso Qt (":/..."), que só pode ser lido.
bool isResourcePath(const QString &path);

// Caminho de destino de um salvamento. Sem extensão, recebe 'defaultSuffix'; nesse caso o caminho
// final não foi o escolhido, e sobrescrever um arquivo existente com ele é recusado (o diálogo só
// confirmou o nome sem extensão).
schema::OperationResult resolveSavePath(const QString &pathOrUrl, const QString &defaultSuffix, QString &out);

} // namespace bina::app
