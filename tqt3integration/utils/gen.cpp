 /*
 *  This file is part of the Trinity Desktop Environment
 *
 *  Original file taken from the OpenSUSE tdebase builds
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <tqfile.h>
#include <tqstring.h>
#include <tqvaluelist.h>
#include <stdlib.h>

// TODO includes, forwards


/*

FUNCTION <name>
 RETURN_TYPE <type>
 DELAYED_RETURN   - use DCOP transaction in kded module, function will take some time to finish
 SKIP_TQT - don't generate in tqt file
 ONLY_TQT - generate only in tqt file
 ADD_APPINFO - generate wmclass arguments
 ARG <name>
  TYPE <type>
  ORIG_TYPE <type>          - for example when the function accepts TQWidget*, but WId is really used
  ORIG_CONVERSION <conversion>
  IGNORE
  NEEDS_DEREF
  CONST_REF
  OUT_ARGUMENT
  CONVERSION <function>
  BACK_CONVERSION <function> - for out arguments
  CREATE <function>  - doesn't exist in TQt, create in tqttde using function
  PARENT - the argument is a parent window to be used for windows
 ENDARG
ENDFUNCTION

*/

struct Arg
    {
    Arg() : ignore( false ), needs_deref( false ), const_ref( false ), out_argument( false ), parent( false ) {}
    TQString name;
    TQString type;
    TQString orig_type;
    TQString orig_conversion;
    bool ignore;
    bool needs_deref;
    bool const_ref;
    bool out_argument;
    TQString conversion;
    TQString back_conversion;
    TQString create;
    bool parent;
    };

struct Function
    {
    Function() : delayed_return( false ), skip_tqt( false ), only_tqt( false ), add_appinfo( false ) {}
    TQString name;
    TQString return_type;
    bool delayed_return;
    bool skip_tqt;
    bool only_tqt;
    bool add_appinfo;
    TQValueList< Arg > args;
    void stripNonOutArguments();
    void stripCreatedArguments();
    };

void Function::stripNonOutArguments()
    {
    TQValueList< Arg > new_args;
    for( TQValueList< Arg >::ConstIterator it = args.begin();
         it != args.end();
         ++it )
        {
        const Arg& arg = (*it);
        if( arg.out_argument )
            new_args.append( arg );
        }
    args = new_args;
    }

void Function::stripCreatedArguments()
    {
    TQValueList< Arg > new_args;
    for( TQValueList< Arg >::ConstIterator it = args.begin();
         it != args.end();
         ++it )
        {
        const Arg& arg = (*it);
        if( arg.create.isEmpty())
            new_args.append( arg );
        }
    args = new_args;
    }

TQValueList< Function > functions;

TQFile* input_file = NULL;
TQTextStream* input_stream = NULL;
static TQString last_line;
int last_lineno = 0;

#define check( arg ) my_check( __FILE__, __LINE__, arg )
#define error() my_error( __FILE__, __LINE__ )

void my_error( const char* file, int line )
    {
    fprintf( stderr, "Error: %s: %d\n", file, line );
    fprintf( stderr, "Line %d: %s\n", last_lineno, last_line.utf8().data());
    abort();
    }

void my_check( const char* file, int line, bool arg )
    {
    if( !arg )
        my_error( file, line );
    }

void openInputFile( const TQString& filename )
    {
    check( input_file == NULL );
    input_file = new TQFile( filename );
    printf("[INFO] Reading bindings definitions from file %s\n", filename.ascii());
    if( !input_file->open( IO_ReadOnly ))
        error();
    input_stream = new TQTextStream( input_file );
    last_lineno = 0;
    }

TQString getInputLine()
    {
    while( !input_stream->atEnd())
        {
        TQString line = input_stream->readLine().stripWhiteSpace();
        ++last_lineno;
        last_line = line;
        if( line.isEmpty() || line[ 0 ] == '#' )
            continue;
        return line;
        }
    return TQString();
    }

void closeInputFile()
    {
    delete input_stream;
    delete input_file;
    input_stream = NULL;
    input_file = NULL;
    }

void parseArg( Function& function, const TQString& details )
    {
    Arg arg;
    arg.name = details;
    TQString line = getInputLine();
    while( !line.isNull() )
        {
        if( line.startsWith( "ENDARG" ))
            {
            check( !arg.type.isEmpty());
            function.args.append( arg );
            return;
            }
        else if( line.startsWith( "TYPE" ))
            {
            check( arg.type.isEmpty());
            arg.type = line.mid( strlen( "TYPE" )).stripWhiteSpace();
            }
        else if( line.startsWith( "ORIG_TYPE" ))
            {
            check( arg.orig_type.isEmpty());
            arg.orig_type = line.mid( strlen( "ORIG_TYPE" )).stripWhiteSpace();
            }
        else if( line.startsWith( "ORIG_CONVERSION" ))
            {
            check( arg.orig_conversion.isEmpty());
            arg.orig_conversion = line.mid( strlen( "ORIG_CONVERSION" )).stripWhiteSpace();
            }
        else if( line.startsWith( "IGNORE" ))
            {
            check( !arg.out_argument );
            arg.ignore = true;
            }
        else if( line.startsWith( "NEEDS_DEREF" ))
            {
            check( !arg.const_ref );
            arg.needs_deref = true;
            }
        else if( line.startsWith( "CONST_REF" ))
            {
            check( !arg.needs_deref );
            check( !arg.out_argument );
            arg.const_ref = true;
            }
        else if( line.startsWith( "OUT_ARGUMENT" ))
            {
            check( !arg.ignore );
            check( !arg.const_ref );
            arg.out_argument = true;
            }
        else if( line.startsWith( "CONVERSION" ))
            {
            check( arg.conversion.isEmpty());
            arg.conversion = line.mid( strlen( "CONVERSION" )).stripWhiteSpace();
            }
        else if( line.startsWith( "BACK_CONVERSION" ))
            {
            check( arg.back_conversion.isEmpty());
            arg.back_conversion = line.mid( strlen( "BACK_CONVERSION" )).stripWhiteSpace();
            }
        else if( line.startsWith( "CREATE" ))
            {
            check( arg.create.isEmpty());
            arg.create = line.mid( strlen( "CREATE" )).stripWhiteSpace();
            }
        else if( line.startsWith( "PARENT" ))
            {
            arg.parent = true;
            }
        else
            error();
        line = getInputLine();
        }
    error();
    }

void parseFunction( const TQString& details )
    {
    Function function;
    function.name = details;
    TQString line = getInputLine();
    while( !line.isNull() )
        {
        if( line.startsWith( "ENDFUNCTION" ))
            {
            if( function.add_appinfo )
                {
                Arg arg;
                arg.name = "wmclass1";
                arg.type = "TQCString";
                arg.const_ref = true;
                arg.create = "tqAppName";
                function.args.append( arg );
                arg.name = "wmclass2";
                arg.create = "tqAppClass";
                function.args.append( arg );
                }
            check( !function.return_type.isEmpty());
            functions.append( function );
            return;
            }
        else if( line.startsWith( "RETURN_TYPE" ))
            {
            check( function.return_type.isEmpty());
            function.return_type = line.mid( strlen( "RETURN_TYPE" )).stripWhiteSpace();
            }
        else if( line.startsWith( "DELAYED_RETURN" ))
            function.delayed_return = true;
        else if( line.startsWith( "SKIP_TQT" ))
            function.skip_tqt = true;
        else if( line.startsWith( "ONLY_TQT" ))
            function.only_tqt = true;
        else if( line.startsWith( "ADD_APPINFO" ))
            function.add_appinfo = true;
        else if( line.startsWith( "ARG" ))
            {
            parseArg( function, line.mid( strlen( "ARG" )).stripWhiteSpace());
            }
        else
            error();
        line = getInputLine();
        }
    error();
    }

void parse(TQString filename)
    {
    openInputFile( filename );
    TQString line = getInputLine();
    while( !line.isNull() )
        {
        if( line.startsWith( "FUNCTION" ))
            {
            parseFunction( line.mid( strlen( "FUNCTION" )).stripWhiteSpace());
            }
        else
            error();
        line = getInputLine();
        }
    closeInputFile();
    }

TQString makeIndent( int indent )
    {
    return indent > 0 ? TQString().fill( ' ', indent ) : "";
    }

void generateFunction( TQTextStream& stream, const Function& function, const TQString name,
    int indent, bool staticf, bool orig_type, bool ignore_deref, int ignore_level )
    {
    TQString line;
    line += makeIndent( indent );
    if( staticf )
        line += "static ";
    line += function.return_type + " " + name + "(";
    bool need_comma = false;
    for( TQValueList< Arg >::ConstIterator it = function.args.begin();
         it != function.args.end();
         ++it )
        {
        const Arg& arg = (*it);
        if( ignore_level >= 2 && arg.ignore )
            continue;
        if( need_comma )
            {
            line += ",";
            if( line.length() > 80 )
                {
                stream << line << "\n";
                line = makeIndent( indent + 4 );
                }
            else
                line += " ";
            }
        else
            line += " ";
        need_comma = true;
        if( orig_type && !arg.orig_type.isEmpty())
            line += arg.orig_type;
        else
            {
            if( arg.const_ref )
                line += "const ";
            line += arg.type;
            if( !ignore_deref && arg.needs_deref )
                line += "*";
            if( arg.const_ref )
                line += "&";
            }
        if( ignore_level >= 1 && arg.ignore )
            line += " /*" + arg.name + "*/";
        else
            line += " " + arg.name;
        }
    line += " )";
    stream << line;
    }

void generateTQtH()
    {
    TQFile file( "tqttdeintegration_x11_p.h.gen" );
    if( !file.open( IO_WriteOnly ))
        error();
    TQTextStream stream( &file );
    for( TQValueList< Function >::ConstIterator it = functions.begin();
         it != functions.end();
         ++it )
        {
        Function f = *it;
        if( f.skip_tqt )
            continue;
        f.stripCreatedArguments();
        generateFunction( stream, f, f.name, 8,
            true /*static*/, true /*orig type*/, false /*ignore deref*/, 0 /*ignore level*/ );
        stream << ";\n";
        }
    }

void generateTQtCpp()
    {
    TQFile file( "tqttdeintegration_x11.cpp.gen" );
    if( !file.open( IO_WriteOnly ))
        error();
    TQTextStream stream( &file );
    for( TQValueList< Function >::ConstIterator it = functions.begin();
         it != functions.end();
         ++it )
        {
        Function f = *it;
        if( f.only_tqt )
            continue;
        f.stripCreatedArguments();
        generateFunction( stream, f, "(*tqttde_" + f.name + ")", 0,
            true /*static*/, false /*orig type*/, false /*ignore deref*/, 0 /*ignore level*/ );
        stream << ";\n";
        }
    stream <<
"\n"
"void TQTDEIntegration::initLibrary()\n"
"    {\n"
"    if( !inited )\n"
"        {\n"
"        enable = false;\n"
"        inited = true;\n"
"        TQString libpath = findLibrary();\n"
"        if( libpath.isEmpty())\n"
"            return;\n"
"        TQLibrary lib( libpath );\n"
"        if( !TQFile::exists( lib.library())) // avoid stupid TQt warning\n"
"            return;\n"
"        lib.setAutoUnload( false );\n";
    for( TQValueList< Function >::ConstIterator it = functions.begin();
         it != functions.end();
         ++it )
        {
        Function function = *it;
        if( function.only_tqt )
            continue;
        stream << makeIndent( 8 ) + "tqttde_" + function.name + " = (\n";
        function.stripCreatedArguments();
        generateFunction( stream, function, "(*)", 12,
            false /*static*/, false /*orig type*/, false /*ignore deref*/, 0 /*ignore level*/ );
        stream << "\n" + makeIndent( 12 ) + ")\n";
        stream << makeIndent( 12 ) + "lib.resolve(\"" + (*it).name + "\");\n";
        stream << makeIndent( 8 ) + "if( tqttde_" + (*it).name + " == NULL )\n";
        stream << makeIndent( 12 ) + "return;\n";
        }
    stream <<
"        enable = tqttde_initializeIntegration();\n"
"        }\n"
"    }\n"
"\n";
    for( TQValueList< Function >::ConstIterator it1 = functions.begin();
         it1 != functions.end();
         ++it1 )
        {
        Function function = *it1;
        if( function.skip_tqt || function.only_tqt )
            continue;
        function.stripCreatedArguments();
        generateFunction( stream, function, "TQTDEIntegration::" + function.name, 0,
            false /*static*/, true /*orig type*/, false /*ignore deref*/, 0 /*ignore level*/ );
        stream << "\n";
        stream << makeIndent( 4 ) + "{\n";
        stream << makeIndent( 4 ) + "return tqttde_" + function.name + "(\n";
        stream << makeIndent( 8 );
        bool need_comma = false;
        for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
             it2 != function.args.end();
             ++it2 )
            {
            const Arg& arg = (*it2);
            if( need_comma )
                stream << ", ";
            need_comma = true;
            if( !arg.orig_conversion.isEmpty())
                {
                stream << arg.orig_conversion + "( " + arg.name + " )";
                }
            else
                stream << arg.name;
            }
        stream << " );\n";
        stream << makeIndent( 4 ) + "}\n";
        }
    }

void generateTQt()
    {
    generateTQtH();
    generateTQtCpp();
    }

void generateTQtTDE()
    {
    TQFile file( "tqttde_functions.cpp" );
    if( !file.open( IO_WriteOnly ))
        error();
    TQTextStream stream( &file );
    for( TQValueList< Function >::ConstIterator it1 = functions.begin();
         it1 != functions.end();
         ++it1 )
        {
        const Function& function = *it1;
        if( function.only_tqt )
            continue;
        Function stripped_function = function;
        stripped_function.stripCreatedArguments();
        stream << "extern \"C\"\n";
        generateFunction( stream, stripped_function, stripped_function.name, 0,
            false /*static*/, false /*orig type*/, false /*ignore deref*/, 1 /*ignore level*/ );
        stream << "\n";
        stream <<
"    {\n"
"    if( tqt_xdisplay() != NULL )\n"
"        XSync( tqt_xdisplay(), False );\n";
        TQString parent_arg;
        for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
             it2 != function.args.end();
             ++it2 )
            {
            const Arg& arg = (*it2);
            if( arg.ignore )
                continue;
            if( arg.parent )
                {
                parent_arg = arg.name;
                break;
                }
            }
        if( !parent_arg.isEmpty())
            {
            stream << "    if( " << parent_arg << " == 0 )\n";
            stream << "        DCOPRef( \"kded\", \"MainApplication-Interface\" ).call( \"updateUserTimestamp\", tqt_x_time );\n";
            }
        stream <<
"    TQByteArray data, replyData;\n"
"    TQCString replyType;\n";
        if( !function.args.isEmpty())
            {
            stream << "    TQDataStream datastream( data, IO_WriteOnly );\n";
            stream << "    datastream";
            for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
                 it2 != function.args.end();
                 ++it2 )
                {
                const Arg& arg = (*it2);
                if( arg.ignore )
                    continue;
                stream << " << ";
                if( !(arg.conversion).isNull() )
                    stream << arg.conversion + "( ";
                if( !arg.create.isEmpty())
                    stream << arg.create + "()";
                else
                    {
                    if( arg.needs_deref )
                        stream << "( " << arg.name << " != NULL ? *" << arg.name << " : " << arg.type << "())";
                    else
                        stream << arg.name;
                    }
                if( !(arg.conversion).isNull() )
                    stream << " )";
                }
            stream << ";\n";
            }
        stream << "    if( !dcopClient()->call( \"kded\", \"tdeintegration\",\"" + function.name + "(";
        bool need_comma = false;
        for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
             it2 != function.args.end();
             ++it2 )
            {
            const Arg& arg = (*it2);
            if( arg.ignore )
                continue;
            if( need_comma )
                stream << ",";
            need_comma = true;
            stream << arg.type;
            }
        stream << ")\", data, replyType, replyData, true ))\n";
        stream << "        {\n";
        if( function.return_type != "void" )
            {
            stream << "        " + function.return_type << " ret;\n";
            stream << "        dcopTypeInit( ret ); // set to false/0/whatever\n";
            stream << "        return ret;\n";
            }
        else
            stream << "        return;\n";
        stream << "        }\n";
        bool return_data = false;
        for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
             !return_data && it2 != function.args.end();
             ++it2 )
            {
            if( (*it2).out_argument )
                return_data = true;
            }
        if( return_data || function.return_type != "void" )
            stream << "    TQDataStream replystream( replyData, IO_ReadOnly );\n";
        if( function.return_type != "void" )
            {
            stream << "    " + function.return_type << " ret;\n";
            stream << "    replystream >> ret;\n";
            }
        if( return_data )
            {
            for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
                 it2 != function.args.end();
                 ++it2 )
                {
                const Arg& arg = (*it2);
                if( arg.out_argument && arg.needs_deref )
                    stream << "    " << arg.type << " " << arg.name + "_dummy;\n";
                }
            stream << "    replystream";
            for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
                 it2 != function.args.end();
                 ++it2 )
                {
                const Arg& arg = (*it2);
                if( arg.out_argument )
                    {
                    stream << " >> ";
                    if( !(arg.back_conversion).isNull() )
                        stream << arg.name + "_dummy";
                    else
                        {
                        if( arg.needs_deref )
                            stream << "( " << arg.name << " != NULL ? *" << arg.name << " : " << arg.name << "_dummy )";
                        else
                            stream << arg.name;
                        }
                    }
                }
            stream << ";\n";
            for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
                 it2 != function.args.end();
                 ++it2 )
                {
                const Arg& arg = (*it2);
                if( arg.out_argument && (!(arg.back_conversion).isNull()) )
                    stream << "    if( " << arg.name << " != NULL )\n"
                        << makeIndent( 8 ) << "*" << arg.name << " = " << arg.back_conversion << "( " << arg.name + "_dummy );\n";
                }
            }
        if( function.return_type != "void" )
            stream << "    return ret;\n";
        stream << "    }\n";
        stream << "\n";
        }
    }
    
void genarateTDEDcop( TQTextStream& stream )
    {
    stream <<
"bool Module::process(const TQCString &fun, const TQByteArray &data,\n"
"    TQCString &replyType, TQByteArray &replyData)\n"
"    {\n";
    for( TQValueList< Function >::ConstIterator it1 = functions.begin();
         it1 != functions.end();
         ++it1 )
        {
        const Function& function = *it1;
        if( function.only_tqt )
            continue;
        stream << "    if( fun == \"" + function.name + "(";
        bool need_comma = false;
        for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
             it2 != function.args.end();
             ++it2 )
            {
            const Arg& arg = (*it2);
            if( arg.ignore )
                continue;
            if( need_comma )
                stream << ",";
            need_comma = true;
            stream << arg.type;
            }
        stream << ")\" )\n";
        stream << "        {\n";
        if( function.delayed_return )
            stream << "        pre_" + function.name + "( data );\n";
        else
            {
            stream << "        pre_" + function.name + "( data, replyData );\n";
            stream << "        replyType = \"" << function.return_type << "\";\n";
            }
        stream << "        return true;\n";
        stream << "        }\n";
        }
    stream <<
"    return KDEDModule::process( fun, data, replyType, replyData );\n"
"    }\n"
"\n";
    stream <<
"QCStringList Module::functions()\n"
"    {\n"
"    QCStringList funcs = KDEDModule::functions();\n";
    for( TQValueList< Function >::ConstIterator it1 = functions.begin();
         it1 != functions.end();
         ++it1 )
        {
        const Function& function = *it1;
        if( function.only_tqt )
            continue;
        stream << "    funcs << \"" + function.name + "(";
        bool need_comma = false;
        for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
             it2 != function.args.end();
             ++it2 )
            {
            const Arg& arg = (*it2);
            if( arg.ignore )
                continue;
            if( need_comma )
                stream << ",";
            need_comma = true;
            stream << arg.type;
            }
        stream << ")\";\n";
        }
    stream <<
"    return funcs;\n"
"    }\n"
"\n"
"QCStringList Module::interfaces()\n"
"    {\n"
"    QCStringList ifaces = KDEDModule::interfaces();\n"
"    ifaces << \"TDEIntegration\";\n"
"    return ifaces;\n"
"    }\n"
"\n";
    }

void genarateTDEPreStub( TQTextStream& stream )
    {
    for( TQValueList< Function >::ConstIterator it1 = functions.begin();
         it1 != functions.end();
         ++it1 )
        {
        const Function& function = *it1;
        if( function.only_tqt )
            continue;
        stream << "void Module::pre_" + function.name + "( const TQByteArray& "
            + ( function.args.isEmpty() ? "" : "data" )
            + ( function.delayed_return ? "" : ", TQByteArray& replyData" )
            + " )\n";
        stream << "    {\n";
        if( function.delayed_return )
            {
            stream << "    JobData job;\n";
            stream << "    job.transaction = tdeApp->dcopClient()->beginTransaction();\n";
            stream << "    job.type = JobData::" + TQString( function.name[ 0 ].upper()) + function.name.mid( 1 ) + ";\n";
            }
        for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
             it2 != function.args.end();
             ++it2 )
            {
            const Arg& arg = (*it2);
            if( arg.ignore )
                continue;
            stream << "    " + arg.type + " " + arg.name + ";\n";
            }
        if( !function.args.isEmpty())
            {
            stream << "    TQDataStream datastream( data, IO_ReadOnly );\n";
            stream << "    datastream";
            for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
                 it2 != function.args.end();
                 ++it2 )
                {
                const Arg& arg = (*it2);
                if( arg.ignore )
                    continue;
                stream << " >> " + arg.name;
                }
            stream << ";\n";
            }
        if( function.delayed_return )
            stream << "    void* handle = " + function.name + "( ";
        else
            stream << "    post_" + function.name + "( " + function.name + "( ";
        bool need_comma = false;
        for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
             it2 != function.args.end();
             ++it2 )
            {
            const Arg& arg = (*it2);
            if( arg.ignore )
                continue;
            if( need_comma )
                stream << ", ";
            need_comma = true;
            stream << arg.name;
            }
        if( function.delayed_return )
            {
            stream << " );\n";
            stream << "    jobs[ handle ] = job;\n";
            }
        else
            stream << " ), replyData );\n";
        stream << "    }\n";
        stream << "\n";
        }
    }

void genarateTDEPostStub( TQTextStream& stream )
    {
    for( TQValueList< Function >::ConstIterator it1 = functions.begin();
         it1 != functions.end();
         ++it1 )
        {
        const Function& function = *it1;
        if( function.only_tqt )
            continue;
        stream << "void Module::post_" + function.name + "( ";
        bool needs_comma = false;
        if( function.delayed_return )
            {
            stream << "void* handle";
            needs_comma = true;
            }
        if( function.return_type != "void" )
            {
            if( needs_comma )
                stream << ", ";
            needs_comma = true;
            stream << function.return_type + " ret";
            }
        for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
             it2 != function.args.end();
             ++it2 )
            {
            const Arg& arg = (*it2);
            if( arg.out_argument )
                {
                if( needs_comma )
                    stream << ", ";
                needs_comma = true;
                stream << arg.type + " " + arg.name;
                }
            }
        if( !function.delayed_return )
            stream << ( needs_comma ? "," : "" ) << " TQByteArray& replyData";
        stream << " )\n";
        stream << "    {\n";
        if( function.delayed_return )
            {
            stream << "    assert( jobs.contains( handle ));\n";
            stream << "    JobData job = jobs[ handle ];\n";
            stream << "    jobs.remove( handle );\n";
            stream << "    TQByteArray replyData;\n";
            stream << "    TQCString replyType = \"tqttde\";\n";
            }
        bool return_data = false;
        for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
             !return_data && it2 != function.args.end();
             ++it2 )
            {
            if( (*it2).out_argument )
                return_data = true;
            }
        if( function.return_type != "void" || return_data )
            stream << "    TQDataStream replystream( replyData, IO_WriteOnly );\n";
        if( function.return_type != "void" )
            stream << "    replystream << ret;\n";
        if( return_data )
            {
            stream << "    replystream";
            for( TQValueList< Arg >::ConstIterator it2 = function.args.begin();
                 it2 != function.args.end();
                 ++it2 )
                {
                const Arg& arg = (*it2);
                if( arg.out_argument )
                    stream << " << " + arg.name;
                }
            stream << ";\n";
            }
        if( function.delayed_return )
            stream << "    tdeApp->dcopClient()->endTransaction( job.transaction, replyType, replyData );\n";
        stream << "    }\n";
        stream << "\n";
        }
    }

void genarateTDEStubs( TQTextStream& stream )
    {
    genarateTDEPreStub( stream );
    genarateTDEPostStub( stream );
//    TODO udelat i predbezne deklarace pro skutecne funkce?
    }

void genarateTDECpp()
    {
    TQFile file( "module_functions.cpp" );
    if( !file.open( IO_WriteOnly ))
        error();
    TQTextStream stream( &file );
    genarateTDEDcop( stream );
    genarateTDEStubs( stream );
    }

void genarateTDEH()
    {
    TQFile file( "module_functions.h" );
    if( !file.open( IO_WriteOnly ))
        error();
    TQTextStream stream( &file );
    for( TQValueList< Function >::ConstIterator it1 = functions.begin();
         it1 != functions.end();
         ++it1 )
        {
        const Function& function = *it1;
        if( function.only_tqt )
            continue;
        Function real_function = function;
        if( function.delayed_return )
            real_function.return_type = "void*";
        generateFunction( stream, real_function, real_function.name, 8,
            false /*static*/, false /*orig type*/, true /*ignore deref*/, 2 /*ignore level*/ );
        stream << ";\n";
        stream << makeIndent( 8 ) + "void pre_" + function.name + "( const TQByteArray& data"
            + ( function.delayed_return ? "" : ", TQByteArray& replyData" ) + " );\n";
        Function post_function = function;
        post_function.stripNonOutArguments();
        if( function.return_type != "void" )
            {
            Arg return_arg;
            return_arg.name = "ret";
            return_arg.type = function.return_type;
            post_function.args.prepend( return_arg );
            }
        if( function.delayed_return )
            {
            Arg handle_arg;
            handle_arg.name = "handle";
            handle_arg.type = "void*";
            post_function.args.prepend( handle_arg );
            }
        else
            {
            Arg handle_arg;
            handle_arg.name = "replyData";
            handle_arg.type = "TQByteArray&";
            post_function.args.append( handle_arg );
            }
        post_function.return_type = "void";
        generateFunction( stream, post_function, "post_" + post_function.name, 8,
            false /*static*/, false /*orig type*/, true /*ignore deref*/, 2 /*ignore level*/ );
        stream << ";\n";
        }
    }

void genarateTDE()
    {
    genarateTDECpp();
    genarateTDEH();
    }

void generate()
    {
    generateTQt();
    generateTQtTDE();
    genarateTDE();
    }

int main (int argc, char *argv[])
    {
    if (argc > 1) {
        parse(TQString(argv[1]));
    }
    else {
        parse(TQString("gen.txt"));
    }
    generate();
    return 0;
    }
