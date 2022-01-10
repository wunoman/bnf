#ifndef __llua_h__
#define __llua_h__
#include "bnf.h"

using namespace bnf;

namespace llua {

#define cdo "do"
#define cif "if"
#define cin "in"
#define cor "or"

#define cand "and"
#define cend "end"
#define cfor "for"
#define cnil "nil"
#define cnot "not"

#define cthen "then"
#define celse "else"
#define ctrue "true"
#define cgoto "goto"

#define clocal "local"
#define cbreak "break"
#define cfalse "false"
#define cuntil "until"
#define cwhile "while"

#define celseif "elseif"
#define crepeat "repeat"
#define creturn "return"

#define cfunction "function"

//----------------------------------------------------------------------------------------------
roadmap String_F( Nfunc<uchar> &node );
roadmap String_FRS( const Symbol &symbol );

roadmap Comment_F( Nfunc<uchar> &node );
roadmap Comment_FRS( const Symbol &symbol );

roadmap OPERATOR_F( Nfunc<uchar> &node );
roadmap OPERATOR_FRS( const Symbol &symbol );

//----------------------------------------------------------------------------------------------
enum NODE_ID {
  nid_S,
  nid_elem,
  nid_name,
  nid_luastring,
  nid_quotestring,
  nid_doublequotestring,
  nid_string,
  nid_number,
  nid_realnumber,
  nid_space,
  nid_comment,
  nid_operator,
  //符号部分
  nid_identity,    // ==
  nid_assign,      // =
  nid_notidentity, // ~=
  nid_plus,
  nid_subtract,
  nid_multiply,
  nid_divide,
  nid_mod,
  nid_joining,
  nid_semicolon,
  nid_colon,
  nid_floordivide, // floor divide //
  nid_bitand,
  nid_bitor,
  nid_bitnot,
  nid_bitright,
  nid_bitleft,
  nid_power,
  nid_greatequal,
  nid_great,
  nid_lessequal,
  nid_less,
  nid_comma,
  nid_dot,
  nid_leftsquarebrackets,
  nid_rightsquarebrackets,
  nid_leftparenthesis,
  nid_rightparenthesis,
  nid_leftbrace,
  nid_rightbrace,
  nid_sharp,       // #
  nid_ellipse,     // ...
  nid_doublecolon, // ::
  //
  nid_kwand,
  nid_kwbreak,
  nid_kwdo,
  nid_kwelse,
  nid_kwelseif,
  nid_kwend,
  nid_kwfalse,
  nid_kwfor,
  nid_kwfunction,
  nid_kwif,
  nid_kwin,
  nid_kwlocal,
  nid_kwnil,
  nid_kwnot,
  nid_kwor,
  nid_kwrepeat,
  nid_kwreturn,
  nid_kwthen,
  nid_kwtrue,
  nid_kwuntil,
  nid_kwwhile,
  nid_kwgoto,
};

enum AST_ID {
  nidNone = 0,
  nidChunk,

  nidBlock,
  nidBlockOptionStat,
  nidBlockOptionRetstat,

  nidExpList,
  nidExpListCommanExp,

  nidPrefixExp,
  nidPrefixExpTail,
  nidPrefixExpIndexExp,
  nidPrefixExpIndexName,
  nidPrefixExpNameBegin,
  nidPrefixExpExpBegin,
  nidPrefixExpExpInParenthesis,

  nidVar,
  nidVarExpIndexExp,
  nidVarExpIndexName,
  nidVarExpInParenthesis,
  nidVarCallOrIndexByDot,
  nidVarCallOrIndexBySquare,
  nidVarPostfixCallOrIndexByDotPending,
  nidVarPostfixCallOrIndexByDotCanBeEmpty,
  nidVarPostfixCallOrIndexByDotCanBeEmptySquareIndex,
  nidVarPostfixCallOrIndexByDot,
  nidVarPostfixCallOrIndexBySquarePending,
  nidVarPostfixCallOrIndexBySquareCanBeEmpty,
  nidVarPostfixCallOrIndexBySquareCanBeEmptyDotIndex,

  nidVarPrefixNameCallOrDotIndex,
  nidVarPrefixParenthesisExpCallOrDotIndex,
  nidVarPrefixNameCallOrSquareIndex,
  nidVarPrefixParenthesisExpCallOrSquareIndex,

  nidArgs,
  nidArgsExplist,
  nidArgsExplistOptionExplist,

  nidStat,

  nidRetstat,
  nidRetstatOptionExpList,
  nidRetstatOptionSemiColon,

  nidLabel,
  nidBinop,
  nidUnop,
  nidName,
  nidNil,
  nidFalse,
  nidTrue,
  nidNumber,
  nidRealNumber,
  nidLuaString,
  nidQuoteString,
  nidDoubleQuoteString,
  nidString,
  nidEllipse,
  nidDot,
  nidFor,
  nidIn,
  nidRepeat,
  nidUntil,
  nidWhile,
  nidBreak,
  nidDo,
  nidIf,
  nidThen,
  nidElse,
  nidElseIf,
  nidLocal,
  nidColon,
  nidFunction,
  nidEnd,
  nidSemiColon,
  nidReturn,
  nidComma,
  nidAssign,
  nidLeftSquareBrackets,
  nidRightSquareBrackets,
  nidLeftParenthesis,
  nidRightParenthesis,

  nidLeftBrace,
  nidRightBrace,

  nidForIn,

  nidForDo,
  nidForDoStep,

  nidWhileDo,
  nidRepeatUntil,

  nidFunctionDefine,

  nidLocalFuncDefine,

  nidIfThen,
  nidIfThenOptionElseIf,
  nidIfThenOptionElseBlock,

  nidDoEnd,
  nidLocalNameList,
  nidLocalNameListOption,

  nidTableConstructor,
  nidTableConstructorOptionFieldList,

  nidExp,
  nidExpRegular,
  nidExpRegularPre,
  nidExpRegularExp,
  nidExpBinExp,
  nidExpUnExp,

  nidField,

  nidFieldsep,
  nidFieldExplicit,
  nidFieldIdentity,

  nidFieldList,
  nidFieldListSepField,
  nidFieldListFieldSep,
  nidFieldListOptionFieldSep,

  nidVarList,
  nidVarListOptionVar,

  nidVarListAssign,

  nidGoto,
  nidGotoName,

  nidFuncName,
  nidFuncNameOptionName,
  nidFuncNameDotName,

  nidFunctionDef,

  nidFuncBody,
  nidFuncBodyOptionParList,

  nidFuncCall,
  nidFuncCallSelfArgs,
  nidFuncCallIndexName,

  nidFuncCallIndexExp,
  nidFuncCallParenthesisExp,
  nidFuncCallSelfCallOrIndex,
  nidFuncCallPostfixSelfCallOrIndexPending,
  nidFuncCallPostfixSelfCallOrIndexCanBeEmpty,
  nidFuncCallPostfixSelfCallOrIndex,
  nidFuncCallPostfixSelfCallOrIndexCanBeEmptyArgs,
  nidFuncCallCallOrIndex,
  nidFuncCallPostfixCallOrIndexPending,
  nidFuncCallPostfixCallOrIndexCanBeEmpty,
  nidFuncCallPostfixCallOrIndexCanBeEmptySelfCall,

  nidNameList,
  nidNameListTail,

  nidParList,
  nidParListNameList,
  nidParListNameListOptionEllipse,
};

////----------------------------------------------------------------------------------------------
//template <NODE_ID nid, AST_ID AID> roadmap kw_f( Sfunc &node ) {
  //auto rdm = roadmap::unmatched;
  //assert( node.pSource );
  //auto &source = *node.pSource;
  //auto tk = Token{};
  //if ( source.get( tk ) ) {
    //switch ( tk.nid ) {
      //case nid:
        //rdm = roadmap::matched;
        //node.node_id = AID;
        //break;
    //}
  //}
  //if ( rdm == roadmap::unmatched ) {
    //source.setposition( node.match_bi );
  //}
  //return rdm;
//}

//template <NODE_ID nid> roadmap kw_frs( const Symbol &symbol ) {
  //return (symbol.token.nid==nid) ? roadmap::matched : roadmap::unmatched;
//}

//----------------------------------------------------------------------------------------------

roadmap label_f( Sfunc &node );
roadmap label_frs( const Symbol &symbol );

roadmap binop_f( Sfunc &node );
roadmap binop_frs( const Symbol &symbol );

roadmap unop_f( Sfunc &node );
roadmap unop_frs( const Symbol &symbol );

roadmap fieldsep_f( Sfunc &node );
roadmap fieldsep_frs( const Symbol &symbol );

roadmap expregularpre_f( Sfunc &node );
roadmap expregularpre_frs( const Symbol &symbol );

roadmap string_f( Sfunc &node );
roadmap string_frs( const Symbol &symbol );

//----------------------------------------------------------------------------------------------

bnf::Sseque &create_funcname();
bnf::Sseque &create_gotoname();
bnf::Sseque &create_forin( bnf::Sseque &namelist, bnf::Sseque &explist, bnf::Sseque &block );
bnf::Sseque &create_functiondefine( bnf::Sseque &funcname, bnf::Sseque &funcbody );
bnf::Sseque &create_localfuncdefine( bnf::Sseque &funcbody );
bnf::Sseque &create_repeatuntil( bnf::Sseque &block, bnf::Salter &exp );
bnf::Sseque &create_ifthen( bnf::Sseque &block, bnf::Salter &exp );
bnf::Sseque &create_varlist( bnf::Salter &var );
bnf::Sseque &create_varlistassign( bnf::Sseque &varlist, bnf::Sseque &explist );
bnf::Sseque &create_whiledo( bnf::Salter &exp, bnf::Sseque &block );
bnf::Sseque &create_fordo( bnf::Salter &exp, bnf::Sseque &block );
bnf::Salter &create_field( bnf::Salter **exp );
bnf::Sseque &create_fieldlist( bnf::Salter &field );
bnf::Sseque &create_tableconstructor( bnf::Sseque &fieldlist );
bnf::Sseque &create_namelist();
bnf::Salter &create_parlist( bnf::Sseque &namelist );
bnf::Sseque &create_block( bnf::Salter **stat, bnf::Sseque &retstat );
bnf::Sseque &create_retstat( bnf::Sseque &explist );
bnf::Sseque &create_explist( bnf::Salter &exp );
bnf::Sseque &create_functiondef( bnf::Sseque &funcbody );
bnf::Sseque &create_funcbody( bnf::Salter &parlist, bnf::Sseque &block );
bnf::Salter &create_var( bnf::Salter &exp, bnf::Salter &args );
bnf::Salter &create_args( bnf::Sseque &explist, bnf::Sseque &tableconsntructor );
bnf::Salter &create_funccall( bnf::Salter &exp, bnf::Salter &args );
bnf::Sseque &create_doend( bnf::Sseque &block );
bnf::Sseque &create_localnamelist( bnf::Sseque &namelist, bnf::Sseque &explist );
void setup_exp( bnf::Salter &exp, bnf::Sseque &functiondef, bnf::Salter **prefixexp,
                bnf::Sseque &tableconstructor );
void setup_prefixexp( bnf::Salter &prefixexp, bnf::Salter &exp, bnf::Salter &args );

//----------------------------------------------------------------------------------------------
void filter( bnf::Result &result, bnf::tokens_t &dest );
int parse_ast( bnf::Source_token &source, bnf::Stack<Token> &stack, bnf::Result &result,
               bnf::Parser<Token> &parser );
// int lex( bnf::Source<uchar> &src, bnf::Stack<bnf::uchar> &stack, bnf::Result &result );
roadmap lex( bnf::Parser<uchar> &parser );
int lex_markkeyword( bnf::Source<uchar> &src, bnf::Result &result );
roadmap parse_file( bnf::cch *fn );

} // namespace llua

#endif
