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
#include <qtextstream.h>
#include <qdebug.h>
#include "decompress.h"

/*
  Callgrind files are compressed by default.  To make it easier to read the
  callgrind files it is usefull to turn off the compression.  You can do this
  with the valgrind options "--compress-strings=no --compress-pos=no" or you
  can use this tool.  For more information about the compression see the
  valgrind documentation.

  http://valgrind.org/docs/manual/cl-format.html
*/
int main(int argc, const char *argv[])
{
    QFile inFile;
    QFile outFile;
    QTextStream in(&inFile);
    QTextStream out(&outFile);

    // Either read in from stdin and output to stdout
    // or read in from a file and output to a file
    // Example: ./app callgrind.out.111 -o callgrind.out.111.u
    bool setInput = false;
    bool setOutput = false;
    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "-o") {
            setOutput = true;
        } else if (setOutput) {
            outFile.setFileName(arg);
            outFile.open(QIODevice::WriteOnly);
        } else if (QFile::exists(arg)) {
            setInput = true;
            inFile.setFileName(arg);
            inFile.open(QIODevice::ReadOnly);
        } else {
            qWarning() << "Usage:" << argv[0]
                        << "[stdin|callgrindFile]" << "[stdout|-o outFile]" << i;
            return 1;
        }
    }
    if (!setInput) inFile.open(stdin, QIODevice::ReadOnly);
    if (!setOutput) outFile.open(stdout, QIODevice::WriteOnly);
    if (inFile.openMode() == QIODevice::NotOpen
        || outFile.openMode() == QIODevice::NotOpen)
        return 1;

    Decompress decompress;
    do {
        out << decompress.process(in.readLine()) << '\n';
    } while (!in.atEnd());

    return 0;
}

