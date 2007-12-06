/*
* Copyright (C) 2007 Benjamin C Meyer
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * The name of the contributors may not be used to endorse or promote
*     products derived from this software without specific prior
*     written permission.
*
* THIS SOFTWARE IS PROVIDED BY <copyright holder> ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "decompress.h"

#include <qstringlist.h>
#include <stdlib.h>

#define FN 0
#define FL 1
#define OB 2
inline int normalizeType(const QStringRef &name)
{
    QStringRef normal = name;
    if (normal.at(0) == 'c')
        normal = QStringRef(name.string(), 1, name.length() - 1);
    switch(normal.at(1).toAscii()){
    case 'n':
        return FN;
    case 'l':
    case 'i':
    case 'e':
        return FL;
    case 'b':
        return OB;
    }
    Q_ASSERT(true);
    return 0;
}

QString Decompress::process(const QString &originalLine)
{
    if (originalLine.isEmpty())
        return originalLine;

    QString line = originalLine;
    switch(line.at(0).toAscii()){
    case 'f':
    case 'c':
    case 'o': {
        // Name compression
        int eq = line.indexOf(QLatin1Char('='));
        int par = line.indexOf(QLatin1Char(')'));
        if (eq != -1 && par != -1 && line[eq + 1] == QLatin1Char('(')) {
            QStringRef idType(&line, 0, eq);
            QString type = line.mid(eq + 2, par - eq - 2);
            int id = atoi(type.toLatin1().constData());
            QStringRef name(&line, par + 1, line.length() - par - 1);

            block b;
            b.type = normalizeType(idType);
            b.id = id;
            bool isFunction = (line.startsWith(QLatin1String("fn=")));
            if (name.isEmpty()) {
                QList<block>::iterator i =
                    qBinaryFind(names.begin(), names.end(), b);
                Q_ASSERT_X(names.end() - i != 0,
                           "find", "Id and Type Not Found!");
                line = idType.toString() + QLatin1String("=") + i->name;
            } else {
                name = QStringRef(&line, name.position() + 1, name.length() - 1);
                b.name = name.toString();
                QList<block>::iterator i =
                    qLowerBound(names.begin(), names.end(), b);
                names.insert(i, b);
                line = idType.toString() + QLatin1String("=") + b.name;
            }
            if (isFunction) {
                currentFunctionId = b.id;
                currentLineNumber = -1;
            }
        }
    }
    default :
        // Number compression
        if (currentFunctionId != -1) {
            if (currentLineNumber < 0) {
                bool ok;
                currentLineNumber = line.mid(0, line.indexOf(QLatin1Char(' '))).toInt(&ok);
                if (!ok)
                    currentLineNumber = -1;
            } else {
                QStringList words = line.split(QLatin1Char(' '), QString::SkipEmptyParts);
                bool changed = false;
                for (int i = 0; i < words.count(); ++i) {
                    int newLine = currentLineNumber;
                    switch(words[i][0].toAscii()){
                    case '*':
                        break;
                    case '+':
                    case '-':
                        newLine += words.at(i).toInt();
                        break;
                    default:
                        continue;
                    }
                    changed = true;
                    words[i] = QString::number(newLine);
                    // changing the line number only occurs on first column
                    if (i == 0)
                        currentLineNumber = newLine;
                }
                if (changed)
                    line = words.join(QLatin1String(" "));
            }
        }
    }
    return line;
}

