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

#include "compiler.h"

#include "pp.h"
#include "binder.h"
#include "default_visitor.h"

#include <qfile.h>
#include <qdir.h>
#include <qdebug.h>
#include <qprocess.h>

class Watcher: protected DefaultVisitor
{
public:
    QList<int> lines;
    QList<int> returns;
    QList<int> jumps;
    QList<QPair<int, int> > functionCalls;
    QList<QPair<int, int> > functions;
    QString m_file;

  Watcher(LocationManager &__location, const QString &file)
   :m_file(file),
    _M_location(__location),
    _M_token_stream(&_M_location.token_stream)
    {}

  FileModelItem run(AST *node) {
      visit(node);
      FileModelItem result;// = _M_current_file;
      return result;
  }

protected:
    virtual void visitIfStatement(IfStatementAST *node)
    {
       // p(node);
       DefaultVisitor::visitIfStatement(node);
    }

    virtual void visit(AST *node)
    {
      p(node);
      DefaultVisitor::visit(node);
    }

    int p(AST *node)
    {
        if (node) {
            if (node->kind == AST::Kind_CompoundStatement
                || node->kind == AST::Kind_StringLiteral)
                return 0;
          QString filename;
          int line = 0;
          int column = 0;
          assert (node != 0);
          _M_location.positionAt(_M_token_stream->position(node->start_token), &line, &column, &filename);
          if (filename.contains(m_file)) {
            int columnEnd, lineEnd;
            _M_location.positionAt(_M_token_stream->position(node->end_token), &lineEnd, &columnEnd, &filename);
            if (!lines.contains(line))
                lines.append(line);
            //if (line == 974 && line != lineEnd)
                //qDebug() << "break:" << line << lineEnd << column << columnEnd << node->kind << AST::Kind_FunctionDefinition;
                 const Token &start_token = _M_token_stream->token((int) node->start_token);
                  const Token &end_token = _M_token_stream->token((int) node->end_token);
                 QString elt = QString::fromUtf8(&start_token.text[start_token.position],
                                           (int) (end_token.position - start_token.position)).trimmed();
            //if (line == 974 && line != lineEnd)
            //    qDebug() <<  filename << ":" << line << lineEnd << column << columnEnd << node->kind << elt;
            if (node->kind == AST::Kind_ReturnStatement)
                if (!returns.contains(line))
                    returns.append(line);
            if (node->kind == AST::Kind_FunctionDefinition) {
                //if (!functions.contains(line))
                    functions.append(QPair<int, int>(line, -1));
            }
            if (node->kind == AST::Kind_InitDeclarator) {
                if (!functions.isEmpty()
                    && functions.last().second == -1
                    && functions.last().first != -1) {
                    functions[functions.count() - 1].second = lineEnd;
                    //qDebug() << functions[functions.count() - 1].first << functions[functions.count() - 1].second;
                }
            }
            if (node->kind == AST::Kind_FunctionCall) {
                functionCalls.append(QPair<int,int>(line, lineEnd));
            }

          }
         return line;
      }
      return 0;
    }

private:

private:
  LocationManager &_M_location;
  TokenStream *_M_token_stream;
};

static bool preprocess(const QString &sourceFile, QByteArray *out)
{
    rpp::pp_environment env;
    rpp::pp preprocess(env);

    rpp::pp_null_output_iterator null_out;

    const char *ppconfig = ":/trolltech/generator/parser/rpp/pp-qt-configuration";

    QFile configFile(ppconfig);
    if (!configFile.open(QFile::ReadOnly)) {
        fprintf(stderr, "Preprocessor configuration file not found '%s'\n", ppconfig);
        return false;
    }

    QByteArray ba = configFile.readAll();
    configFile.close();
    preprocess.operator() (ba.constData(), ba.constData() + ba.size(), null_out);

    QString qtdir = getenv ("QTDIR");
    if (qtdir.isEmpty()) {
        fprintf(stderr, "Generator requires QTDIR to be set\n");
        return false;
    }

    qtdir += "/include";

    QString currentDir = QDir::current().absolutePath();
    QFileInfo sourceInfo(sourceFile);
    QDir::setCurrent(sourceInfo.absolutePath());

    preprocess.push_include_path(".");
    preprocess.push_include_path(QDir::convertSeparators(qtdir).toStdString());
    preprocess.push_include_path(QDir::convertSeparators(qtdir + "/QtXml").toStdString());
    preprocess.push_include_path(QDir::convertSeparators(qtdir + "/QtNetwork").toStdString());
    preprocess.push_include_path(QDir::convertSeparators(qtdir + "/QtCore").toStdString());
    preprocess.push_include_path(QDir::convertSeparators(qtdir + "/QtGui").toStdString());
    preprocess.push_include_path(QDir::convertSeparators(qtdir + "/QtOpenGL").toStdString());

    std::string result;
    result.reserve (20 * 1024); // 20K

    result += "# 1 \"builtins\"\n";
    result += "# 1 \"";
    result += sourceFile.toStdString();
    result += "\"\n";

    preprocess.file(sourceInfo.fileName().toStdString(), 
                        rpp::pp_output_iterator<std::string>(result));

    *out = QString::fromStdString(result).toUtf8();
    return true;
}

Compiler::Compiler(const QString &file) : parser(&control)
{
    loadFile(file);
}

void Compiler::loadFile(const QString &file)
{
    QProcess process;
    QStringList arguments;
    arguments << "-E" << file;
    process.start("gcc", arguments);
    process.waitForFinished();
    contents = process.readAllStandardOutput();

    //if (!preprocess(file, &contents))
    //    return;
    ast = parser.parse(contents, contents.size(), &__pool);

/*
    QFile ff(".pp");
    ff.open(QFile::WriteOnly);
    ff.write(contents);
    ff.close();
    */
}

QList<int> Compiler::linesTouched(const QString &file)
{
    Watcher watcher(parser.location(), file);
    watcher.run(ast);
    return watcher.lines;
}

QList<int> Compiler::returns(const QString &file)
{
    Watcher watcher(parser.location(), file);
    watcher.run(ast);
    return watcher.returns;
}

QList<QPair<int, int> > Compiler::functions(const QString &file)
{
    Watcher watcher(parser.location(), file);
    watcher.run(ast);
    return watcher.functions;
}
