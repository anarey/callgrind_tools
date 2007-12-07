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

#include <qfile.h>
#include <qstack.h>
#include <qtextstream.h>
#include <qdebug.h>
#include <qstringlist.h>

#include "decompress.h"

/*
    A small tool to extract information out of a callgrind file
*/
int main(int argc, const char *argv[])
{
    QFile inFile;
    QTextStream in(&inFile);
    QTextStream out(stdout);

    bool spec = false;
    bool functions = false;
    bool cost = false;
    QStringList args;

    // Either read in from stdin or read in from a file
    bool setInput = false;
    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (i == 1 && QFile::exists(arg)) {
            setInput = true;
            inFile.setFileName(arg);
            inFile.open(QIODevice::ReadOnly);
        } else if (arg == "-spec") {
            spec = true;
        } else if (arg == "-functions") {
            functions = true;
        } else if (arg == "-cost") {
            cost = true;
        } else if (spec || functions || cost) {
            args += arg;
        } else {
            qWarning() << "Usage:" << argv[0]
                        << "[stdin|callgrindFile]" << "[options]";
            qWarning() << "-functions file";
            qWarning() << "-cost function";
            qWarning() << "-spec type [ob|fl|fn|fe|calls]";
            return 1;
        }
    }
    if (!setInput) inFile.open(stdin, QIODevice::ReadOnly);
    if (inFile.openMode() == QIODevice::NotOpen) {
        qWarning() << "Unable to open callgrind file";
        return 1;
    }

    QStringList seen;
    QStack<QString> currentFile;
    bool next = false;
    int totalCost = 0;
    Decompress decompress;
    do {
        QString line = decompress.process(in.readLine());
        if (spec && line.startsWith(args.first() + "=") && !seen.contains(line)) {
            seen.append(line);
            out << line.mid(line.indexOf("=") + 1) << endl;
        } else if (functions) {
            if (line.startsWith("fl=")) {
                if(!currentFile.isEmpty())
                    currentFile.pop();
                currentFile.push(line.mid(line.indexOf("=") + 1));
            }
            if (line.startsWith("cfi=")) {
                currentFile.push(line.mid(line.indexOf("=") + 1));
            }
            if (line.startsWith("cfn") || line.startsWith("fn")) {
                if (currentFile.top() == args.first()) {
                    QString fn = line.mid(line.indexOf("=") + 1);
                    if (!seen.contains(fn)) {
                        seen.append(fn);
                        out << fn << endl;
                    }
                }
                if(line.startsWith("cfn") && currentFile.size() == 2)
                    currentFile.pop();
            }
        } else if (cost) {
            if (line.startsWith("fn=")) {
                bool was = next;
                next = (line == ("fn=" + args.first()));
                if (was && !next)
                    break;
            }
            if (next) {
                QStringList list = line.split(' ');
                if (list.count() == 2 && !line.startsWith("calls")) {
                    bool ok;
                    int lineCost = list[1].toInt(&ok);
                    if (ok)
                        totalCost += lineCost;
                }
            }
        }
    } while (!in.atEnd());
    if (cost)
        out << totalCost << endl;
    return 0;
}

