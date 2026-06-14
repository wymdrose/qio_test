#include "xlsx_load.h"

#include <QDir>
#include <QFile>
#include <QTemporaryFile>
#include <QUuid>

#include <regex>
#include <stdexcept>

#include <cstring>

#include <zip.h>

namespace FileIo
{
namespace {

QString makeTempXlsxPath(const QString &prefix)
{
    return QDir::temp().filePath(
        QStringLiteral("%1_%2.xlsx")
            .arg(prefix, QUuid::createUuid().toString(QUuid::WithoutBraces)));
}

QByteArray zipPathBytes(const QString &path)
{
    return QDir::toNativeSeparators(path).toLocal8Bit();
}

std::string sanitizeWorkbookXml(const std::string &xml)
{
    static const std::regex badDefinedName(
        R"(<definedName[^>]*>[^<]*#REF![^<]*</definedName>)",
        std::regex::icase);
    std::string result = std::regex_replace(xml, badDefinedName, std::string());
    if (result.find("#REF!") != std::string::npos)
    {
        static const std::regex allDefinedNames(R"(<definedNames>.*?</definedNames>)", std::regex::icase);
        result = std::regex_replace(result, allDefinedNames, "<definedNames/>");
    }
    return result;
}

bool isRepairableLoadError(const QString &message)
{
    return message.contains(QStringLiteral("#REF!"))
        || message.contains(QStringLiteral("bad cell coordinates"));
}

void loadWorkbookFromPath(xlnt::workbook &workbook, const QString &path)
{
#if defined(Q_OS_WIN)
    workbook.load(path.toStdWString());
#else
    workbook.load(path.toUtf8().constData());
#endif
}

bool sanitizeXlsxDefinedNames(const QString &sourcePath, const QString &destPath, QString *error)
{
    const QByteArray src = zipPathBytes(sourcePath);
    const QByteArray dst = zipPathBytes(destPath);

    int zipError = 0;
    zip_t *archive = zip_open(src.constData(), ZIP_RDONLY, &zipError);
    if (!archive)
    {
        if (error)
            *error = QStringLiteral("Failed to open Excel archive (code %1)").arg(zipError);
        return false;
    }

    zip_t *output = zip_open(dst.constData(), ZIP_CREATE | ZIP_TRUNCATE, &zipError);
    if (!output)
    {
        zip_close(archive);
        if (error)
            *error = QStringLiteral("Failed to create repaired Excel file (code %1)").arg(zipError);
        return false;
    }

    const zip_int64_t entryCount = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < entryCount; ++i)
    {
        const char *name = zip_get_name(archive, i, 0);
        if (!name)
            continue;

        zip_stat_t stat {};
        if (zip_stat_index(archive, i, 0, &stat) != 0)
            continue;

        zip_source_t *source = zip_source_zip_file(
            output, archive, static_cast<zip_uint64_t>(i), 0, 0, -1, nullptr);
        if (!source)
            continue;

        if (std::string(name) == "xl/workbook.xml")
        {
            zip_file_t *file = zip_fopen_index(archive, i, 0);
            if (file)
            {
                std::string xml(static_cast<std::size_t>(stat.size), '\0');
                const zip_int64_t readBytes = zip_fread(file, xml.data(), stat.size);
                zip_fclose(file);

                if (readBytes > 0)
                {
                    xml.resize(static_cast<std::size_t>(readBytes));
                    xml = sanitizeWorkbookXml(xml);
                    zip_source_free(source);
                    auto *buffer = static_cast<char *>(malloc(xml.size()));
                    if (!buffer)
                        continue;
                    std::memcpy(buffer, xml.data(), xml.size());
                    source = zip_source_buffer(output, buffer, xml.size(), 1);
                }
            }
        }

        if (!source)
            continue;

        const zip_int64_t newIndex = zip_file_add(output, name, source, ZIP_FL_OVERWRITE);
        if (newIndex < 0)
            zip_source_free(source);
    }

    const bool saved = zip_close(output) == 0;
    zip_close(archive);

    if (!saved && error)
        *error = QStringLiteral("Failed to save repaired Excel file");

    return saved;
}

} // namespace

bool loadWorkbook(xlnt::workbook &workbook, const QString &path, QString *error)
{
    try
    {
        loadWorkbookFromPath(workbook, path);
        return true;
    }
    catch (const std::exception &e)
    {
        const QString message = QString::fromUtf8(e.what());
        if (!isRepairableLoadError(message))
        {
            if (error)
                *error = message;
            return false;
        }
    }

    const QString sourceCopyPath = makeTempXlsxPath(QStringLiteral("qiotest_src"));
    if (!QFile::copy(path, sourceCopyPath))
    {
        if (error)
            *error = QStringLiteral("Failed to copy Excel file for repair");
        return false;
    }

    const QString tempPath = makeTempXlsxPath(QStringLiteral("qiotest_xlsx"));
    if (!sanitizeXlsxDefinedNames(sourceCopyPath, tempPath, error))
    {
        QFile::remove(sourceCopyPath);
        return false;
    }
    QFile::remove(sourceCopyPath);

    try
    {
        loadWorkbookFromPath(workbook, tempPath);
        QFile::remove(tempPath);
        return true;
    }
    catch (const std::exception &e)
    {
        QFile::remove(tempPath);
        if (error)
            *error = QString::fromUtf8(e.what());
        return false;
    }
}

} // namespace FileIo
