#include "csvparser.h"

QStringList CsvParser::parseLine(const QString &line)
{
    QStringList result;
    for (int offset = 0; offset < line.size(); ++offset) {
        QString value = parseValue(line, offset);
        if (value.isNull()) {
            break;
        }
        result.append(value);
    }
    return result;
}

QString CsvParser::parseValue(const QString &line, int &offset)
{
    QString result;
    if (offset >= line.size()) {
        return result;
    }
    bool startsWithsQuote = '"' == line[offset];
    if (startsWithsQuote) {
        ++offset;
    }
    while (offset < line.size()) {
        QChar c = line[offset];
        ++offset;
        if (',' == c) {
            if (!startsWithsQuote) {
                break;
            }
        } else if ('"' == c) {
            if (startsWithsQuote) {
                break;
            }
        }
        result.append(c);
    }
    return result;
}
