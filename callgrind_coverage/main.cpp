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
#include "compiler.h"

#include <qdebug.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qstringlist.h>
#include <qsettings.h>
#include <qdir.h>

QStringList readSourceFile(const QString &sourceFile)
{
    QStringList lines;
    QFile file(sourceFile);
    file.open(QFile::ReadOnly);
    QTextStream stream(&file);
    QString line;
    do {
        line = stream.readLine();
        lines.append(line);
    } while (!line.isNull());
    return lines;
}

int main(int argc, const char **argv)
{
    if (argc < 4) {
        qWarning() << "usage:" << argv[0] << "callgrindFile sourceFile supressionFile";
        return 1;
    }

    QString callgrindFile = argv[1];
    QString sourceFile = argv[2];
    QString suppressionsFile = argv[3];

    if (!QFile::exists(sourceFile)) {
        qWarning() << "source file don't exists";
        return 1;
    }
    if (!QFile::exists(callgrindFile)) {
        qWarning() << "callgrind file don't exists";
        return 1;
    }
    QFileInfo file(sourceFile);
    QString sourceFileName = file.fileName();

    CallgrindFile cf(callgrindFile);
    QList<int> touchedLines = cf.linesTouched(sourceFileName);
    QList<CallgrindFile::jump> jumps = cf.jumps(sourceFileName);

    QDir::setCurrent(file.dir().absolutePath());
    Compiler compiler(sourceFileName);
    QList<int> compiledLines = compiler.linesTouched(sourceFileName);
    QList<int> returns = compiler.returns(sourceFileName);
    QList<QPair<int, int> > functions = compiler.functions(sourceFileName);
    QStringList sourceLines = readSourceFile(sourceFileName);

    //QSettings suppressions(suppressionsFile, QSettings::IniFormat);
    //suppressions.setGroup("jumps");
    // keys = settings.value("jumps").toString().split(",");

    QString skip = ".*Q_ASSERT.*";
    for (int i = jumps.count() - 1; i >= 0; --i) {
        //qDebug() << jumps[i].from << sourceLines.value(jumps[i].from - 1) << jumps[i].to;
        if (sourceLines.value(jumps[i].from - 1).contains(QRegExp(skip)))
            jumps[i].fromCount = 1;
    }

    QList<int> skips;
    skip = ".*Q_UNUSED.*";
    for (int i = 0 ; i < sourceLines.count(); ++i) {
        if (sourceLines.value(i).contains(QRegExp(skip)))
            skips.append(i + 1);
    }

    QTextStream out(stdout);

    out << "Function coverage - Has each function in the program been executed?" << endl;
    int touchedFunctions = 0;
    for (int i = 0; i < functions.count(); ++i) {
        int start = functions.at(i).first;
        int end = functions.at(i).second;
        for (int j = 0; j < touchedLines.count(); ++j) {
            if (touchedLines.at(j) >= start
                && touchedLines.at(j) <= end) {
                touchedFunctions++;
                // remove extra lines from functions which are often on multiple lines
                //out << "does touch: " << sourceLines[loc - 1] << endl;
                for (int k = start; k < end - 1; ++k) {
                    //qDebug() << "removing" << k << start << end;
                    compiledLines.removeAll(k);
                }
                start = -1;
                break;
            }
            //qDebug() << touchedLines.at(j);
        }
        if (start != -1)
            out << "\tNot executed: " << sourceLines[start - 1] << endl;
    }
    out << "\tFunctions: " << touchedFunctions << "/" << functions.count()
        << " " << (int)((touchedFunctions/(double)functions.count()) * 100) << "%" << endl;


    out << "Statement coverage - Has each line of the source code been executed?" << endl;
    out << "\tExecuted: " << touchedLines.count() << "/" << compiledLines.count()
        << " " << (int)((touchedLines.count()/(double)compiledLines.count()) * 100) << "%" << endl;

    out << "Condition coverage - Has each evaluation point (such as a true/false decision) been executed?" << endl;
    int touchedConditions = 0;
    for (int i = 0; i < jumps.count(); ++i) {
        if (jumps[i].fromCount != 0) touchedConditions++;
        else out << "\t" << "always false " << jumps[i].from << " " << jumps[i].to << " " << jumps[i].toCount << " " << sourceLines[jumps[i].from - 1] << endl;
        if (jumps[i].toCount != 0) touchedConditions++;
        else out << "\t" << "always true " << jumps[i].from << " " << jumps[i].fromCount << " " << jumps[i].to << " " << sourceLines[jumps[i].from - 1] << endl;
        //out << "\t" << jumps[i].from << " " << jumps[i].fromCount << " " << jumps[i].to << sourceLines[jumps[i].from - 1] << endl;
    }
    out << "\tExecuted: " << touchedConditions << "/" << jumps.count()*2
        << " " << (int)((touchedConditions/(double)(jumps.count()*2)) * 100) << "%" << endl;

    // Path coverage - Has every possible route through a given part of the code been executed?
    // Entry/exit coverage - Has every possible call and return of the function been executed?

    out << endl;

    for (int i = 0; i < sourceLines.count(); ++i) {
        int lineNumber = i + 1;
        QString line = sourceLines[i];

        bool ran = (touchedLines.contains(lineNumber));
        bool com = (compiledLines.contains(lineNumber));
        bool rtn = (returns.contains(lineNumber));
        bool jmp = false;
        bool skip = skips.contains(lineNumber);

        for (int i = 0; i < jumps.count(); ++i) {
            //qDebug() << i << jumps[i].from << jumps[i].to << jumps[i].fromCount << jumps[i].toCount;
            if (!ran
                && rtn
                && jumps[i].from + 1 == lineNumber
                //&& (jumps[i].fromCount == 0 || jumps[i].fromCount == 1 /*TODO suppress*/)
                && jumps[i].to != jumps[i].from
                ) {
                ran = true;
                jmp = true;
            }

            if (jumps[i].from == lineNumber
                && (jumps[i].fromCount == 0 || jumps[i].toCount == 0)) {
                if (jumps[i].fromCount == 0)
                out << lineNumber << " [j]\tf: " << line << endl;
                if (jumps[i].toCount == 0)
                out << lineNumber << " ->" << jumps[i].to << " \t" << line << endl;
            }
            if (jumps[i].from == lineNumber)
            {
                jmp = true;
                break;
            }
        }

        lineNumber++;

        if ((!ran && !com) || ran || skip)
            continue;

        out << lineNumber - 1
            << " ["
            << (rtn? "R" : "")
            << (com? "c" : "")
            << (jmp? "J" : "")
            << (ran ? "t" : "")
            << (skip ? "s" : "")
            << "]\t"
            << (ran ? "t" :
                com ? "c" : "-") << ": " << line << endl;
    }

    return 0;
}

