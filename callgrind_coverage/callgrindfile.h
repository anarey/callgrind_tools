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

#ifndef CALLGRINDFILE_H
#define CALLGRINDFILE_H

#include <qstring.h>
#include <qlist.h>
#include <qmap.h>

class CallgrindFile
{
public:
    struct jump {
        int from;
        int to;
        int fromCount;
        int toCount;
    };

    CallgrindFile(const QString &file);
    void loadFile(const QString &file);
    QString file() const { return m_file; }

    QList<int> linesTouched(const QString &file);
    QList<jump> jumps(const QString &file);

private:
    struct block {
        // fn=(16) foo.cpp
        QString type; // fn
        int id;       // 16
        QString name; // foo.cpp
        QString parentType;
        int parentId;
    };

    struct fn {
        QList<int> lines;
        QList<jump> jumps;
    };

    QString m_file;
    QList<block> names;
    QMap<int /*id*/, fn> functionCosts;
};

#endif // CALLGRINDFILE_H


