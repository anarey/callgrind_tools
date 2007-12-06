/*
* Copyright (C) 2007 Benjamin C Meyer
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * The name of the contributors may not be used to endorse or promote products
*       derived from this software without specific prior written permission.
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

#include "callgrindfile.h"

#include <qmap.h>
#include <QMapIterator>
#include <qfile.h>
#include <qtextstream.h>
#include <qdebug.h>
#include <qstringlist.h>
#include <qregexp.h>

CallgrindFile::CallgrindFile(const QString &file)
{
    loadFile(file);
}

QString normalizeName(const QString &name)
{
    QString normal = name;
    if (normal.at(0) == 'c')
        normal = normal.mid(1);
    if (normal == "fi")
        normal = "fl";
    return normal;
}

void CallgrindFile::loadFile(const QString &callgrindFile)
{
    QFile file(callgrindFile);
    if (!file.open(QFile::ReadOnly))
        return;
    m_file = callgrindFile;
    names.clear();
    functionCosts.clear();

    QTextStream stream(&file);
    QString line;
    QMap<int, QString> FilenameMap;
    QString currentKey;

    QRegExp nameCompression("^([a-z]*)=\\(([0-9]*)\\) (.*)");
    QRegExp parentNameCompression("^([a-z]*)=\\(([0-9]*)\\)");
    int lastName = -2;

    int lineNumber = -1;
    QString lastLine;

    int currentFnLine = 0;
    int id = -1;
    int lastFileId = 0;
    do {
        lastLine = line;
        line = stream.readLine();
        lineNumber++;

        if (line.isEmpty())
            continue;
        if(line.contains(QRegExp("^[a-z]*:.*")))
            continue;

        if (line.contains(nameCompression)) {
            QStringList cap = nameCompression.capturedTexts();
            if (!cap.value(1).isEmpty()) {
                block c;
                c.id = cap.value(2).toInt();
                c.name = cap.value(3);
                c.type = normalizeName(cap.value(1));
                if (lastLine.contains(parentNameCompression)) {
                    cap = parentNameCompression.capturedTexts();
                    c.parentType = normalizeName(cap.value(1));
                    c.parentId = cap.value(2).toInt();
                } else {
                    c.parentType = "fl";
                    c.parentId = lastFileId;
                }
                //qDebug() << "inserting" << c.name << line << c.parentType << c.parentId << lastName << lineNumber;
                names.append(c);
                lastName = lineNumber;
            }
        }

        if (line.startsWith("fl=")) {
            line.contains(parentNameCompression);
            QStringList cap = parentNameCompression.capturedTexts();
            lastFileId = cap.value(2).toInt();
            if (lastFileId < 0)
                qDebug() << line;
            continue;
        }

        if (line.startsWith("fn=")) {
            line.contains(parentNameCompression);
            QStringList cap = parentNameCompression.capturedTexts();
            id = cap.value(2).toInt();
            //qDebug() << "got fn" << line << id;
            continue;
        }

        if (id != -1) {
            // actual lines!
            if (line[0] == '*')
                continue;
            QString first = line.split(" ").value(0);
            if (first[0] == '+')
                currentFnLine += first.mid(1).toInt();
            else if (first[0] == '-')
                currentFnLine -= first.mid(1).toInt();
            else
                if (first.contains(QRegExp("^[0-9]*$")))
                    currentFnLine = first.toInt();

            if (!functionCosts[id].lines.contains(currentFnLine))
                functionCosts[id].lines.append(currentFnLine);
            //qDebug() << id << currentFnLine << line << first;

            if (line.startsWith("jump=")) {
                jump j;
                QRegExp jmp("^jump=([0-9]*) (.*)");
                line.contains(jmp);
                QStringList cap = jmp.capturedTexts();
                line.contains(jmp);
                j.toCount = cap.value(1).toInt();
                QString to = cap.value(2);
                if (to[0] == '+')
                    j.to = currentFnLine + to.mid(1).toInt();
                else if (to[0] == '-')
                    j.to = currentFnLine - to.mid(1).toInt();
                else
                    j.to = currentFnLine;
                j.from = currentFnLine;
                j.fromCount = -1;
                //  qDebug() << j.fromCount << j.toCount << j.to << line;
                functionCosts[id].jumps.append(j);
            }

            if (line.startsWith("jcnd=")) {
                jump j;
                QRegExp jmp("^jcnd=([0-9]*)/([0-9]*) (.*)");
                line.contains(jmp);
                QStringList cap = jmp.capturedTexts();
                line.contains(jmp);
                j.fromCount = cap.value(2).toInt();
                j.toCount = cap.value(1).toInt();
                j.fromCount = j.fromCount - j.toCount;
                QString to = cap.value(3);
                if (to[0] == '+')
                    j.to = currentFnLine + to.mid(1).toInt();
                else if (to[0] == '-')
                    j.to = currentFnLine - to.mid(1).toInt();
                else
                    j.to = currentFnLine;
                j.from = currentFnLine;
                //  qDebug() << j.fromCount << j.toCount << j.to << line;
                functionCosts[id].jumps.append(j);
            }

        }
        continue;
    } while (!line.isNull());
}

QList<int> CallgrindFile::linesTouched(const QString &file)
{
    QList<int> lines;

    QString type;
    int id = 0;

    // Find the block type/id that contains what we are looking for
    QListIterator<block> i(names);
    while (i.hasNext()) {
        block x = i.next();
        if (x.name == file) {
            type = x.type;
            id = x.id;
            break;
        }
    }

    // Find all functions that are children of the file
    while (i.hasNext()) {
        block x = i.next();
        if ((x.type == type && x.id == id)
            || (x.parentType == type &&
                x.parentId == id))
        {
            for (int j = 0; j < functionCosts[x.id].lines.count(); ++j) {
                int line = functionCosts[x.id].lines[j];
                if (!lines.contains(line))
                    lines.append(line);
            }
        }
    }

    return lines;
}

QList<CallgrindFile::jump> CallgrindFile::jumps(const QString &file)
{
    QList<jump> jumps;

    QString type;
    int id = 0;

    // Find the block type/id that contains what we are looking for
    QListIterator<block> i(names);
    while (i.hasNext()) {
        block x = i.next();
        if (x.name == file) {
            type = x.type;
            id = x.id;
            break;
        }
    }

    // Find all functions that are children of the file
    while (i.hasNext()) {
        block x = i.next();
        if ((x.type == type && x.id == id)
            || (x.parentType == type &&
                x.parentId == id))
        {
            jumps += functionCosts[x.id].jumps;
        }
    }

    return jumps;
}
