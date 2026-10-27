#include <QCoreApplication>
#include <QFileInfo>
#include <QUrl>

#include "bina/app/Paths.h"

namespace bina::app {

QString localPath(const QString &pathOrUrl)
{
    if (pathOrUrl.startsWith(QLatin1String("file:")))
        return QUrl(pathOrUrl).toLocalFile();

    // QFile abre recursos por ":/caminho", não pela URL "qrc:/caminho".
    if (pathOrUrl.startsWith(QLatin1String("qrc:")))
        return QLatin1Char(':') + QUrl(pathOrUrl).path();

    return pathOrUrl;
}

bool isResourcePath(const QString &path)
{
    return localPath(path).startsWith(QLatin1String(":/"));
}

schema::OperationResult resolveSavePath(const QString &pathOrUrl, const QString &defaultSuffix, QString &out)
{
    out = localPath(pathOrUrl);

    if (out.isEmpty())
        return schema::OperationResult::failure(QCoreApplication::translate("bina", "Nenhum arquivo escolhido"));

    if (isResourcePath(out))
        return schema::OperationResult::failure(QCoreApplication::translate("bina", "Não é possível gravar em um recurso da aplicação"));

    if (!QFileInfo(out).suffix().isEmpty() || defaultSuffix.isEmpty())
        return schema::OperationResult::success();

    out += QLatin1Char('.') + defaultSuffix;

    if (QFileInfo::exists(out))
        return schema::OperationResult::failure(
            QCoreApplication::translate("bina", "'%1' já existe; escolha o nome com a extensão para substituí-lo").arg(out));

    return schema::OperationResult::success();
}

} // namespace bina::app
