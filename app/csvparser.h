#ifndef CSVPARSER_H
#define CSVPARSER_H

#include <QtCore/QString>
#include <QtCore/QStringList>

class CsvParser
{
public:
    static QStringList parseLine(const QString &line);

private:
    static QString parseValue(const QString &line, int &offset);
};

#endif // CSVPARSER_H
