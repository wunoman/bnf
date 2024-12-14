#include "llua.h"

//----------------------------------------------------------------------------------------------
roadmap llua::lex( bnf::Parser<uchar> &parser ) {
  // lua identifier
  bnf::Nfunc ID{ "ID", bnf::ID_F, bnf::ID_FRS, nid_name };

  // lua space
  bnf::Nfunc SPACE{ "SPACE", bnf::SPACE_F, bnf::SPACE_FRS, nid_space };
  SPACE.r_conf.visible = false;

  // lua string
  bnf::Nfunc LUASTRING{ "LUASTRING", llua::String_F, llua::String_FRS, nid_luastring };
  bnf::Nfunc QUOTESTRING{ "QUOTESTRING", bnf::QUOTESTRING_F, bnf::QUOTESTRING_FRS,
                          nid_quotestring };
  bnf::Nfunc DQUOTESTRING{ "DQUOTESTRING", bnf::DQUOTESTRING_F, bnf::DQUOTESTRING_FRS,
                           nid_doublequotestring };
  bnf::Lalter STRING{ "STRING", nid_string };
  STRING.r_conf.hierarchy = false;
  STRING( LUASTRING, QUOTESTRING, DQUOTESTRING );

  // lua number
  bnf::Nfunc REALNUMBER_NS{ "REALNUMBER", bnf::REALNUMBER_NS_F, bnf::REALNUMBER_NS_FRS,
                            nid_realnumber };
  bnf::Lalter NUMBER{ "NUMBER", nid_number };
  NUMBER.r_conf.hierarchy = false;
  NUMBER( REALNUMBER_NS );

  // lua comment
  bnf::Nfunc COMMENT{ "COMMENT", llua::Comment_F, llua::Comment_FRS, nid_comment };

  // lua operator
  bnf::Nfunc OPERATOR{ "OPERATOR", llua::OPERATOR_F, llua::OPERATOR_FRS, nid_operator };

  // lua lex
  bnf::Lalter ELE{ "ELE", nid_elem, 0, bnf::unlimit };
  ELE.r_conf.hierarchy = false;
  ELE( ID, STRING, COMMENT, SPACE, NUMBER, OPERATOR );

  bnf::Nalter<bnf::uchar> S{ "S", nid_S };
  S( ELE );
  auto G = bnf::pool_tree( S );

  auto ret = parser.parse( G );

  return ret;
}

//----------------------------------------------------------------------------------------------
roadmap llua::parse_file( bnf::cch *fn ) {
  assert( fn );

  bnf::Source_file lex_source( fn );
  bnf::Result lex_result;
  bnf::Stack<bnf::uchar> lex_stack;
  bnf::tokens_t tokens;

  auto rdm = roadmap::success;
  if ( roadmap::success == rdm ) {
    bnf::Parser<bnf::uchar> lex_parser{ lex_source, lex_stack, lex_result };
    lex_parser.name = "lua_lex_parser";
    rdm = llua::lex( lex_parser );
    if ( roadmap::success == rdm ) {
      llua::lex_markkeyword( lex_source, lex_result );
    } else {
      lex_parser.print_matchcursor();
    }

    bnf::print_result( lex_parser );
    llua::filter_space_comment( lex_result, tokens ); // no space and comment
    bnf::print_tokens( tokens );
  }

  if ( roadmap::success == rdm ) {
    bnf::Source_token ast_source( tokens );
    bnf::Result ast_result;
    bnf::Stack<Token> ast_stack;
    bnf::Parser<Token> ast_parser{ ast_source, ast_stack, ast_result };
    ast_parser.name = "lua_ast_parser";
    auto rdm = llua::parse_ast( ast_source, ast_stack, ast_result, ast_parser );
    bnf::print_result( ast_parser );
    if ( rdm == roadmap::fail ) {
      ast_parser.print_matchcursor();
    }
  }
  return rdm;
}

//----------------------------------------------------------------------------------------------
// fieldsep ::= ‘,’ | ‘;’
roadmap llua::fieldsep_f( Nfunc<Token> &node ) {
  auto rdm = roadmap::unmatched;
  auto &source = *node.pSource;
  auto tk = Token{};
  if ( source.get( tk ) ) {
    switch ( tk.nid ) {
      case nid_semicolon:
      case nid_comma:
        rdm = roadmap::matched;
        node.node_id = AST_ID::nidFieldsep;
        break;
    }
  }
  if ( rdm == roadmap::unmatched ) {
    source.setposition( node.match_bi );
  }
  return rdm;
}

roadmap llua::fieldsep_frs( const Symbol &symbol ) {
  auto rdm = roadmap::unmatched;
  switch ( symbol.token.nid ) {
    case nid_semicolon:
    case nid_comma: rdm = roadmap::matched; break;
  }
  return rdm;
}

//----------------------------------------------------------------------------------------------
// String
roadmap llua::string_f( Nfunc<Token> &node ) {
  auto rdm = roadmap::unmatched;
  auto &source = *node.pSource;
  auto tk = Token{};
  if ( source.get( tk ) ) {
    switch ( tk.nid ) {
      case nid_luastring:
      case nid_quotestring:
      case nid_doublequotestring:
        rdm = roadmap::matched;
        node.node_id = AST_ID::nidString;
        break;
    }
  }
  if ( rdm == roadmap::unmatched ) {
    source.setposition( node.match_bi );
  }
  return rdm;
}

roadmap llua::string_frs( const Symbol &symbol ) {
  auto rdm = roadmap::unmatched;
  switch ( symbol.token.nid ) {
    case nid_luastring:
    case nid_quotestring:
    case nid_doublequotestring: rdm = roadmap::matched; break;
  }
  return rdm;
}

//----------------------------------------------------------------------------------------------
namespace llua {

auto in_f = kw_f<NODE_ID::nid_kwin, AST_ID::nidIn>;
auto in_frs = kw_frs<NODE_ID::nid_kwin>;

auto while_f = kw_f<NODE_ID::nid_kwwhile, AST_ID::nidWhile>;
auto for_f = kw_f<NODE_ID::nid_kwfor, AST_ID::nidFor>;
auto repeat_f = kw_f<NODE_ID::nid_kwrepeat, AST_ID::nidRepeat>;
auto until_f = kw_f<NODE_ID::nid_kwuntil, AST_ID::nidUntil>;
auto goto_f = kw_f<NODE_ID::nid_kwgoto, AST_ID::nidGoto>;
auto break_f = kw_f<NODE_ID::nid_kwbreak, AST_ID::nidBreak>;
auto do_f = kw_f<NODE_ID::nid_kwdo, AST_ID::nidDo>;
auto if_f = kw_f<NODE_ID::nid_kwif, AST_ID::nidIf>;
auto then_f = kw_f<NODE_ID::nid_kwthen, AST_ID::nidThen>;
auto else_f = kw_f<NODE_ID::nid_kwelse, AST_ID::nidElse>;
auto elseif_f = kw_f<NODE_ID::nid_kwelseif, AST_ID::nidElseIf>;
auto local_f = kw_f<NODE_ID::nid_kwlocal, AST_ID::nidLocal>;
auto end_f = kw_f<NODE_ID::nid_kwend, AST_ID::nidEnd>;
auto function_f = kw_f<NODE_ID::nid_kwfunction, AST_ID::nidFunction>;
auto return_f = kw_f<NODE_ID::nid_kwreturn, AST_ID::nidReturn>;

auto name_f = kw_f<NODE_ID::nid_name, AST_ID::nidName>;

auto assign_f = kw_f<NODE_ID::nid_assign, AST_ID::nidAssign>;
auto dot_f = kw_f<NODE_ID::nid_dot, AST_ID::nidDot>;
auto colon_f = kw_f<NODE_ID::nid_colon, AST_ID::nidColon>;
auto leftparenthesis_f = kw_f<NODE_ID::nid_leftparenthesis, AST_ID::nidLeftParenthesis>;
auto rightparenthesis_f = kw_f<NODE_ID::nid_rightparenthesis, AST_ID::nidRightParenthesis>;
auto semicolon_f = kw_f<NODE_ID::nid_semicolon, AST_ID::nidSemiColon>;
auto comma_f = kw_f<NODE_ID::nid_comma, AST_ID::nidComma>;
auto ellipse_f = kw_f<NODE_ID::nid_ellipse, AST_ID::nidEllipse>;
auto leftbrace_f = kw_f<NODE_ID::nid_leftbrace, AST_ID::nidLeftBrace>;
auto rightbrace_f = kw_f<NODE_ID::nid_rightbrace, AST_ID::nidRightBrace>;
auto leftsquarebrackets_f = kw_f<NODE_ID::nid_leftsquarebrackets, AST_ID::nidLeftSquareBrackets>;
auto rightsquarebrackets_f = kw_f<NODE_ID::nid_rightsquarebrackets, AST_ID::nidRightSquareBrackets>;
} // namespace llua

//----------------------------------------------------------------------------------------------
// expregularpre,这个只是regular前缀的一部分
// r=>nil | false | true | Number | String | ‘...’ |
roadmap llua::expregularpre_f( Nfunc<Token> &node ) {
  auto rdm = roadmap::unmatched;
  auto &source = *node.pSource;
  auto tk = Token{};
  if ( source.get( tk ) ) {
    AST_ID id = nidNone;
    switch ( tk.nid ) {
      case nid_kwnil:
        id = nidNil;
        node.name = "nil";
        break;
      case nid_kwfalse:
        id = nidFalse;
        node.name = "false";
        break;
      case nid_kwtrue:
        id = nidTrue;
        node.name = "true";
        break;
      case nid_number:
        id = nidNumber;
        node.name = "number";
        break;
      case nid_realnumber:
        id = nidRealNumber;
        node.name = "realnumber";
        break;
      case nid_luastring:
        id = nidLuaString;
        node.name = "luastring";
        break;
      case nid_quotestring:
        id = nidQuoteString;
        node.name = "quotestring";
        break;
      case nid_doublequotestring:
        id = nidDoubleQuoteString;
        node.name = "doublequotestring";
        break;
      case nid_ellipse:
        id = nidEllipse;
        node.name = "ellipse";
        break;
    }
    if ( nidNone != id ) {
      rdm = roadmap::matched;
      node.node_id = id;
    }
  }
  if ( rdm == roadmap::unmatched ) {
    source.setposition( node.match_bi );
  }
  return rdm;
}

roadmap llua::expregularpre_frs( const Symbol &symbol ) {
  auto rdm = roadmap::unmatched;
  switch ( symbol.token.nid ) {
    case nid_kwnil:
    case nid_kwfalse:
    case nid_kwtrue:
    case nid_number:
    case nid_realnumber:
    case nid_luastring:
    case nid_quotestring:
    case nid_doublequotestring:
    case nid_ellipse: rdm = roadmap::matched; break;
  }
  return rdm;
}

//----------------------------------------------------------------------------------------------
// unop ::= ‘-’ | not | ‘#’ | ‘~’
roadmap llua::unop_f( Nfunc<Token> &node ) {
  auto rdm = roadmap::unmatched;
  auto &source = *node.pSource;
  auto tk = Token{};
  if ( source.get( tk ) ) {
    switch ( tk.nid ) {
      case nid_subtract:
      case nid_kwnot:
      case nid_sharp:
      case nid_bitnot:
        rdm = roadmap::matched;
        node.node_id = AST_ID::nidUnop;
        break;
    }
  }
  if ( rdm == roadmap::unmatched ) {
    source.setposition( node.match_bi );
  }
  return rdm;
}

roadmap llua::unop_frs( const Symbol &symbol ) {
  auto rdm = roadmap::unmatched;
  switch ( symbol.token.nid ) {
    case nid_subtract:
    case nid_kwnot:
    case nid_sharp:
    case nid_bitnot: rdm = roadmap::matched; break;
  }
  return rdm;
}

//----------------------------------------------------------------------------------------------
// binop ::=  ‘+’ | ‘-’ | ‘*’ | ‘/’ | ‘//’ | ‘^’ | ‘%’ |
// 		 ‘&’ | ‘~’ | ‘|’ | ‘>>’ | ‘<<’ | ‘..’ |
// 		 ‘<’ | ‘<=’ | ‘>’ | ‘>=’ | ‘==’ | ‘~=’ |
// 		 and | or
roadmap llua::binop_f( Nfunc<Token> &node ) {
  auto rdm = roadmap::unmatched;
  auto &source = *node.pSource;
  auto tk = Token{};
  if ( source.get( tk ) ) {
    switch ( tk.nid ) {
      case nid_plus:
      case nid_subtract:
      case nid_multiply:
      case nid_divide:
      case nid_floordivide:
      case nid_power:
      case nid_mod:
      case nid_bitand:
      case nid_bitnot:
      case nid_bitor:
      case nid_bitright:
      case nid_bitleft:
      case nid_joining:
      case nid_less:
      case nid_lessequal:
      case nid_great:
      case nid_greatequal:
      case nid_identity:
      case nid_notidentity:
      case nid_kwand:
      case nid_kwor:
        rdm = roadmap::matched;
        node.node_id = AST_ID::nidBinop;
        break;
    }
  }
  if ( rdm == roadmap::unmatched ) {
    source.setposition( node.match_bi );
  }
  return rdm;
}

roadmap llua::binop_frs( const Symbol &symbol ) {
  auto rdm = roadmap::unmatched;
  switch ( symbol.token.nid ) {
    case nid_plus:
    case nid_subtract:
    case nid_multiply:
    case nid_divide:
    case nid_floordivide:
    case nid_power:
    case nid_mod:
    case nid_bitand:
    case nid_bitnot:
    case nid_bitor:
    case nid_bitright:
    case nid_bitleft:
    case nid_joining:
    case nid_less:
    case nid_lessequal:
    case nid_great:
    case nid_greatequal:
    case nid_identity:
    case nid_notidentity:
    case nid_kwand:
    case nid_kwor: rdm = roadmap::matched; break;
  }
  return rdm;
}

//----------------------------------------------------------------------------------------------
// label ::= ‘::’ Name ‘::’
roadmap llua::label_f( Nfunc<Token> &node ) {
  auto rdm = roadmap::unmatched;

  enum class S : i32 {
    S0 = 0, // 开始状态
    LABEL1,
    ID,
    LAST,
    ERR = 0x10,
    SUC,
  } state{ S::S0 };

  enum class I : i32 {
    dcl = 0, // double colon ::
    id,
    rest,
    LAST,
  } input{ I::rest };

  // dcl, id, rest
  S N[(i32)S::LAST][(i32)I::LAST] = {
      { S::LABEL1, S::ERR, S::ERR }, // S0
      { S::ERR, S::ID, S::ERR },     // LABEL1
      { S::SUC, S::ERR, S::ERR },    // ID
  };

  auto mapinput = []( Token tk ) -> I {
    // regard eof as rest
    auto ret = I::rest;
    switch ( tk.nid ) {
      case nid_doublecolon: ret = I::dcl; break;
      case nid_name: ret = I::id; break;
      default: ret = I::rest;
    }
    return ret;
  };

  auto ok = true;
  auto &source = *node.pSource;
  auto tk = Token{};
  do {
    ok = source.get( tk );
    input = mapinput( tk );
    state = N[(i32)state][(i32)input];
    // 对于自带边界界定的,不会有读取多余信息的情况; 此举用于统一匹配范围标识[b,e)格式
    source.getposition( node.match_ei );
    if ( state >= S::ERR ) { // ERR or SUC
      break;
    }
  } while ( ok );

  rdm = ( state == S::SUC ) ? roadmap::matched : roadmap::unmatched;
  if ( roadmap::matched == rdm ) {
    node.node_id = AST_ID::nidLabel;
  } else {
    source.setposition( node.match_bi );
  }

  return rdm;
}

roadmap llua::label_frs( const Symbol &symbol ) {
  return ( symbol.token.nid == nid_doublecolon ) ? roadmap::matched : roadmap::unmatched;
}

//----------------------------------------------------------------------------------------------
// fieldlist ::= field {fieldsep field} [fieldsep]
bnf::Sseque &llua::create_fieldlist( bnf::Salter &field ) {
  bnf::Sseque astFieldList{ "fieldlist", AST_ID::nidFieldList };

  bnf::Sseque fieldlist_sepfield{ "fieldlist_sepfield", AST_ID::nidFieldListSepField, 0, -1 };
  bnf::Nfunc fieldsep{ "fieldlist_fieldsep", fieldsep_f, fieldsep_frs, nidFieldListFieldSep };
  // {fieldsep field}
  fieldlist_sepfield( fieldsep, field );

  // [fieldsep]
  bnf::Sseque fieldlist_optionfieldsep{
      "fieldlist_optionfieldsep",
      nidFieldListOptionFieldSep,
      0,
  };
  fieldlist_optionfieldsep( fieldsep );
  astFieldList( field, fieldlist_sepfield, fieldlist_optionfieldsep );

  return ( *POOL_TREE( astFieldList ) );
}

//----------------------------------------------------------------------------------------------
// tableconstructor ::= ‘{’ [fieldlist] ‘}’
bnf::Sseque &llua::create_tableconstructor( bnf::Sseque &fieldlist ) {
  bnf::Sseque astTableConstructor{ "tableconstructor", AST_ID::nidTableConstructor };

  bnf::Sseque tableconstructor_optionfieldlist{ "tableconstructor_optionfieldlist",
                                                nidTableConstructorOptionFieldList, 0, 1 };
  tableconstructor_optionfieldlist( fieldlist );
  bnf::Tfunc<nid_leftbrace, nidLeftBrace> leftbrace{ "leftbrace" };
  bnf::Tfunc<nid_rightbrace, nidRightBrace> rightbrace{ "leftbrace" };
  astTableConstructor( leftbrace, tableconstructor_optionfieldlist, rightbrace );

  auto pnTableConstructor = POOL_TREE( astTableConstructor );
  assert( pnTableConstructor );
  return ( *pnTableConstructor );
}

//----------------------------------------------------------------------------------------------
// namelist ::= Name {‘,’ Name}
bnf::Sseque &llua::create_namelist() {
  bnf::Sseque astNameList{ "namelist", AST_ID::nidNameList };
  bnf::Sseque namelist_tail{ "namelist_tail", nidNameListTail, 0, -1 };
  bnf::Tfunc<nid_comma, nidComma> comma{ "namelist_comma"};
  bnf::Tfunc<nid_name, nidName> Name{ "namelist_Name"};
  namelist_tail( comma, Name );
  astNameList( Name, namelist_tail );
  auto pnNameList = POOL_TREE( astNameList );
  assert( pnNameList );
  return ( *pnNameList );
}

//----------------------------------------------------------------------------------------------
// parlist ::= namelist [‘,’ ‘...’] | ‘...’
bnf::Salter &llua::create_parlist( bnf::Sseque &namelist ) {
  bnf::Salter astParList{ "parlist", AST_ID::nidParList, 0, -1 };
  bnf::Sseque parlist_namelist{ "parlist_namelist", nidParListNameList };
  bnf::Sseque parlist_namelistoptionellipse{ "parlist_namelistoptionellipse",
                                             nidParListNameListOptionEllipse, 0, -1 };
  bnf::Tfunc<nid_comma, nidComma> comma{ "parlist_comma"};
  bnf::Tfunc<nid_ellipse, nidEllipse> ellipse{ "ellipse"};
  parlist_namelistoptionellipse( comma, ellipse );
  parlist_namelist( namelist, parlist_namelistoptionellipse );

  astParList( parlist_namelist, ellipse );
  auto pnParList = POOL_TREE( astParList );
  assert( pnParList );
  return ( *pnParList );
}

//----------------------------------------------------------------------------------------------
// block ::= {stat} [retstat]
bnf::Sseque &llua::create_block( bnf::Salter **stat, bnf::Sseque &retstat ) {
  bnf::Sseque astBlock{ "block", nidBlock };
  bnf::Sseque *pnOptionStat = nullptr;
  {
    bnf::Salter astStat{ "stat", nidStat };
    bnf::Sseque block_optionstat{ "block_optionstat", nidBlockOptionStat, 0, -1 };
    block_optionstat( astStat );
    pnOptionStat = POOL_TREE( block_optionstat );
    assert( pnOptionStat );
    assert( stat );
    *stat = &pnOptionStat->asNalter( 0 );
    assert( *stat );
  }

  bnf::Sseque optionretstat{ "block_optionretstat", nidBlockOptionRetstat, 0, 1 };
  optionretstat( retstat );
  astBlock( *pnOptionStat, optionretstat );
  auto pnBlock = POOL_TREE( astBlock );
  assert( pnBlock );

  return ( *pnBlock );
}

//----------------------------------------------------------------------------------------------
// retstat ::= return [explist] [‘;’]
bnf::Sseque &llua::create_retstat( bnf::Sseque &explist ) {
  bnf::Sseque astRetstat{ "retstat", nidRetstat };
  bnf::Sseque optionexplist{ "retstat_optionexplist", nidRetstatOptionExpList, 0, 1 };
  optionexplist( explist );

  bnf::Sseque optionsemicolon{ "restat_optionsemicolon", nidRetstatOptionSemiColon, 0, 1 };
  bnf::Tfunc<nid_semicolon, nidSemiColon> semicolon{ "retstat_semicolon" };
  optionsemicolon( semicolon );

  // bnf::Nfunc return_{
  bnf::Tfunc<nid_kwreturn, nidReturn> return_{ "retstat_return" };
  astRetstat( return_, optionexplist, optionsemicolon );
  auto pnRetstat = POOL_TREE( astRetstat );
  assert( pnRetstat );
  return ( *pnRetstat );
}

//----------------------------------------------------------------------------------------------
// explist ::= exp {‘,’ exp}
bnf::Sseque &llua::create_explist( bnf::Salter &exp ) {
  bnf::Sseque astExpList{ "explist", nidExpList };
  bnf::Sseque explist_commaexp{ "explist_commaexp", nidExpListCommanExp, 0, -1 };
  bnf::Tfunc<nid_comma, nidComma> comma{ "explist_comma" };
  explist_commaexp( comma, exp );
  astExpList( exp, explist_commaexp );
  auto pnExpList = POOL_TREE( astExpList );
  assert( pnExpList );
  return ( *pnExpList );
}

//----------------------------------------------------------------------------------------------
// field ::= ‘[’ exp ‘]’ ‘=’ exp | Name ‘=’ exp | exp
bnf::Salter &llua::create_field( bnf::Salter **exp ) {
  bnf::Salter astField{ "field", AST_ID::nidField };

  bnf::Tfunc<nid_leftsquarebrackets, nidLeftSquareBrackets> field_lsb{ "field_lsb" };
  bnf::Tfunc<nid_assign, nidAssign> field_assign{ "field_assign" };
  bnf::Tfunc<nid_rightsquarebrackets, nidRightSquareBrackets> field_rsb{ "field_rsb" };
  // ‘[’ exp ‘]’ ‘=’ exp
  bnf::Sseque *pnFieldExplicit = nullptr;
  {
    bnf::Sseque field_explicit{ "field_explicit", AST_ID::nidFieldExplicit };
    bnf::Salter astExp{ "exp", AST_ID::nidExp };
    field_explicit( field_lsb, astExp, field_rsb, field_assign, astExp );
    pnFieldExplicit = POOL_TREE( field_explicit );
    assert( exp );
    *exp = &pnFieldExplicit->asNalter( 1 );
    assert( *exp );
  }
  auto &astExp = **exp;
  auto &field_explicit = *pnFieldExplicit;

  // Name ‘=’ exp
  bnf::Tfunc<nid_name, nidName> Name{ "field_Name" };
  bnf::Sseque field_identity{ "field_identity", AST_ID::nidFieldIdentity };
  field_identity( Name, field_assign, astExp );

  // field ::= ‘[’ exp ‘]’ ‘=’ exp | Name ‘=’ exp | exp
  astField( field_explicit, field_identity, astExp );

  return ( *POOL_TREE( astField ) );
}

//----------------------------------------------------------------------------------------------
// funcname ::= Name {‘.’ Name} [‘:’ Name]
bnf::Sseque &llua::create_funcname() {
  bnf::Sseque astFuncName{ "funcname", AST_ID::nidFuncName };
  bnf::Tfunc<nid_name, nidName> Name{ "funcname_Name" };

  bnf::Sseque dotName{ "funcname_dotName", AST_ID::nidFuncNameDotName, 0, -1 };
  bnf::Tfunc<nid_dot, nidDot> dot{ "funcname_dot" };
  dotName( dot, Name );

  bnf::Sseque optionName{ "funcname_optionName", AST_ID::nidFuncNameOptionName, 0, 1 };
  bnf::Tfunc<nid_colon, nidColon> colon{ "funcname_colon" };
  optionName( colon, Name );

  astFuncName( Name, dotName, optionName );
  auto pnFuncName = POOL_TREE( astFuncName );
  assert( pnFuncName );
  return ( *pnFuncName );
}

//----------------------------------------------------------------------------------------------
// gotoName ::= goto Name
bnf::Sseque &llua::create_gotoname() {
  bnf::Sseque astGotoName{ "gotoname", AST_ID::nidGotoName };
  bnf::Tfunc<nid_name, nidName> Name{ "funcname_Name" };
  bnf::Tfunc<nid_kwgoto, nidGoto> goto_{ "funcname_goto" };
  astGotoName( goto_, Name );
  auto pnGotoName = POOL_TREE( astGotoName );
  assert( pnGotoName );
  return ( *pnGotoName );
}

//----------------------------------------------------------------------------------------------
// repeat block until exp
bnf::Sseque &llua::create_repeatuntil( bnf::Sseque &block, bnf::Salter &exp ) {
  bnf::Sseque astRepeatUntil{ "repeatuntil", AST_ID::nidRepeatUntil };
  bnf::Tfunc<nid_kwrepeat, nidRepeat> repeat{ "repeat"};
  bnf::Tfunc<nid_kwuntil, nidUntil> until{ "until"};
  astRepeatUntil( repeat, block, until, exp );
  auto pnRepeatUntil = POOL_TREE( astRepeatUntil );
  assert( pnRepeatUntil );
  return ( *pnRepeatUntil );
}

//----------------------------------------------------------------------------------------------
// if exp then block {elseif exp then block} [else block] end
bnf::Sseque &llua::create_ifthen( bnf::Sseque &block, bnf::Salter &exp ) {
  bnf::Sseque astIfThen{ "ifthen", AST_ID::nidIfThen };
  bnf::Tfunc<nid_kwif, nidIf> if_{ "if"};
  bnf::Tfunc<nid_kwthen, nidThen> then{ "then"};
  bnf::Tfunc<nid_kwend, nidEnd> end_{ "end"};

  bnf::Sseque optionelseif{ "ifthen_optionelseif", nidIfThenOptionElseIf, 0, -1 };
  bnf::Tfunc<nid_kwelseif, nidElseIf> elseif{ "elseif" };
  optionelseif( elseif, exp, then, block );

  bnf::Sseque optionelseblock{ "ifthen_optionelseblock", nidIfThenOptionElseBlock, 0 };
  bnf::Tfunc<nid_kwelse, nidElse> else_{ "else"};
  optionelseblock( else_, block );

  astIfThen( if_, exp, then, block, optionelseif, optionelseblock, end_ );
  auto pnIfThen = POOL_TREE( astIfThen );
  assert( pnIfThen );
  return ( *pnIfThen );
}

//----------------------------------------------------------------------------------------------
// varlist ‘=’ explist
bnf::Sseque &llua::create_varlistassign( bnf::Sseque &varlist, bnf::Sseque &explist ) {
  bnf::Sseque astVarListAssign{ "varlist", AST_ID::nidVarListAssign };
  bnf::Tfunc<nid_assign, nidAssign> assign{ "assign"};
  astVarListAssign( varlist, assign, explist );
  auto pnVarListAssign = POOL_TREE( astVarListAssign );
  assert( pnVarListAssign );
  return ( *pnVarListAssign );
}

//----------------------------------------------------------------------------------------------
// varlist ::= var {‘,’ var}
bnf::Sseque &llua::create_varlist( bnf::Salter &var ) {
  bnf::Sseque astVarList{ "varlist", AST_ID::nidVarList };
  bnf::Tfunc<nid_comma, nidComma> comma{ "varlist_comma"};
  bnf::Sseque optionvar{ "varlist_optionvar", nidVarListOptionVar, 0, -1 };
  optionvar( comma, var );
  astVarList( var, optionvar );
  auto pnVarList = POOL_TREE( astVarList );
  assert( pnVarList );
  return ( *pnVarList );
}

//----------------------------------------------------------------------------------------------
// while exp do block end
bnf::Sseque &llua::create_whiledo( bnf::Salter &exp, bnf::Sseque &block ) {
  bnf::Sseque astWhileDo{ "whiledo", AST_ID::nidWhileDo };
  bnf::Tfunc<nid_kwwhile, nidWhile> while_{ "while"};
  bnf::Tfunc<nid_kwdo, nidDo> do_{ "do"};
  bnf::Tfunc<nid_kwend, nidEnd> end_{ "end" };
  astWhileDo( while_, exp, do_, block, end_ );
  auto pnWhileDo = POOL_TREE( astWhileDo );
  assert( pnWhileDo );
  return ( *pnWhileDo );
}

//----------------------------------------------------------------------------------------------
// for Name ‘=’ exp ‘,’ exp [‘,’ exp] do block end
bnf::Sseque &llua::create_fordo( bnf::Salter &exp, bnf::Sseque &block ) {
  bnf::Sseque astForDo{ "fordo", AST_ID::nidForDo };
  bnf::Tfunc<nid_kwfor, nidFor> for_{ "for"};
  bnf::Tfunc<nid_kwdo, nidDo> do_{ "do"};
  bnf::Tfunc<nid_kwend, nidEnd> end_{ "end"};
  bnf::Tfunc<nid_assign, nidAssign> assign{ "assign"};
  bnf::Tfunc<nid_comma, nidComma> comma{ "for_comma"};
  bnf::Sseque optionstep{ "fordo_step", nidForDoStep, 0 };
  optionstep( comma, exp );

  bnf::Tfunc<nid_name, nidName> Name{ "fordo_name"};
  astForDo( for_, Name, assign, exp, comma, exp, optionstep, do_, block, end_ );
  auto pnForDo = POOL_TREE( astForDo );
  assert( pnForDo );
  return ( *pnForDo );
}

//----------------------------------------------------------------------------------------------
// for namelist in explist do block end
bnf::Sseque &llua::create_forin( bnf::Sseque &namelist, bnf::Sseque &explist, bnf::Sseque &block ) {
  bnf::Sseque astForIn{ "forin", AST_ID::nidForIn };
  bnf::Tfunc<nid_kwfor, nidFor> for_{ "for"};
  // bnf::Sfunc in_{ "in", llua::in_f, nidIn };
  bnf::Tfunc<nid_kwin, nidIn> in_{ "in" };
  // bnf::Sfunc do_{ "do", llua::do_f, nidDo };
  bnf::Tfunc<nid_kwdo, nidDo> do_{ "do" };
  // bnf::Sfunc end_{ "end", llua::end_f, nidEnd };
  bnf::Tfunc<nid_kwend, nidEnd> end_{ "end" };

  astForIn( for_, namelist, in_, explist, do_, block, end_ );
  auto pnForIn = POOL_TREE( astForIn );
  assert( pnForIn );
  return ( *pnForIn );
}

//----------------------------------------------------------------------------------------------
// function funcname funcbody
bnf::Sseque &llua::create_functiondefine( bnf::Sseque &funcname, bnf::Sseque &funcbody ) {
  bnf::Sseque astFunctionDefine{ "functiondefine", AST_ID::nidFunctionDefine };
  // bnf::Sfunc function{ "functiondefine_function", llua::function_f, nidFunction };
  bnf::Tfunc<nid_kwfunction, nidFunction> function{ "functiondefine_function" };
  astFunctionDefine( function, funcname, funcbody );
  auto pnFunctionDefine = POOL_TREE( astFunctionDefine );
  assert( pnFunctionDefine );
  return ( *pnFunctionDefine );
}

//----------------------------------------------------------------------------------------------
// local function Name funcbody
bnf::Sseque &llua::create_localfuncdefine( bnf::Sseque &funcbody ) {
  bnf::Sseque astLocalFuncDefine{ "localfuncdefine", AST_ID::nidLocalFuncDefine };
  // bnf::Sfunc local{ "localfuncdefine_local", llua::local_f, nidLocal };
  // bnf::Sfunc function{ "localfuncdefine_function", llua::function_f, nidFunction };
  // bnf::Sfunc Name{ "localfuncdefine_Name", llua::name_f, nidName };
  bnf::Tfunc<nid_kwlocal, nidLocal> local{ "localfuncdefine_local" };
  bnf::Tfunc<nid_kwfunction, nidFunction> function{ "localfuncdefine_function" };
  bnf::Tfunc<nid_name, nidName> Name{ "localfuncdefine_Name" };
  astLocalFuncDefine( local, function, Name, funcbody );
  auto pnLocalFuncDefine = POOL_TREE( astLocalFuncDefine );
  assert( pnLocalFuncDefine );
  return ( *pnLocalFuncDefine );
}

//----------------------------------------------------------------------------------------------
// exp ::=  nil | false | true | Number | String | ‘...’ | functiondef |
//     prefixexp | tableconstructor | exp binop exp | unop exp
// 改写成
// E=> r|EbE|uE
// r=>nil | false | true | Number | String | ‘...’ | functiondef | prefixexp | tableconstructor
// b=>binop
// u=>unop
//
// binop ::=  ‘+’ | ‘-’ | ‘*’ | ‘/’ | ‘//’ | ‘^’ | ‘%’ |
// 		 ‘&’ | ‘~’ | ‘|’ | ‘>>’ | ‘<<’ | ‘..’ |
// 		 ‘<’ | ‘<=’ | ‘>’ | ‘>=’ | ‘==’ | ‘~=’ |
// 		 and | or
//
// unop ::= ‘-’ | not | ‘#’ | ‘~’
// 消除E的左递归
// E=>rT | uET
// T=>bET | ε
void llua::setup_exp( bnf::Salter &exp, bnf::Sseque &functiondef, bnf::Salter **prefixexp,
                      bnf::Sseque &tableconstructor ) {
  bnf::Sseque binExp{ "exp_binexptail", nidExpBinExp, 0, -1 };
  bnf::Nfunc binop{ "binop", binop_f, binop_frs, nidBinop };
  binExp( binop, exp );

  bnf::Nfunc expregularpre{ "exp_regularpre", expregularpre_f, expregularpre_frs,
                            nidExpRegularPre };
  bnf::Salter *pnRegular = nullptr;
  {
    bnf::Salter exp_regular{ "exp_regular", nidExpRegular };
    bnf::Salter astPrefixexp{ "prefixexp", AST_ID::nidPrefixExp };
    exp_regular( expregularpre, functiondef, astPrefixexp, tableconstructor );
    pnRegular = POOL_TREE( exp_regular );
    assert( pnRegular );
    assert( prefixexp );
    *prefixexp = &pnRegular->asNalter( 2 );
    assert( *prefixexp );
  }

  // rT
  bnf::Sseque regularExp{ "exp_regularexp", nidExpRegularExp };
  regularExp( *pnRegular, binExp );

  // uET
  bnf::Sfunc unop{ "unop", unop_f, unop_frs, nidUnop };
  bnf::Sseque unExp{ "exp_unexp", nidExpUnExp };
  unExp( unop, exp, binExp );

  exp( regularExp, unExp );
  assert( exp.in_pool() );
  bnf::pool_tree( exp );
}

//----------------------------------------------------------------------------------------------
// prefixexp ::= var | funccall | ‘(’ exp ‘)’
// V==var
// W==prefixexp
// X==funccall
// a==Name
// b=='['exp']'
// c='.'Name
// d=='('exp')'
// e==args
// f==':'Name args
//
// W=>aT | dT
// T=>eT | fT | bT | cT | ε
void llua::setup_prefixexp( bnf::Salter &prefixexp, bnf::Salter &exp, bnf::Salter &args ) {
  // bnf::Nfunc colon{ "colon", llua::colon_f, nidColon };
  bnf::Tfunc<nid_colon, nidColon> colon{ "colon" };
  // bnf::Nfunc a{ "prefixexp_name", llua::name_f, nidName };
  bnf::Tfunc<nid_name, nidName> a{ "prefixexp_name" };
  auto &e = args;

  bnf::Sseque f{ "prefixexp_funccallselfargs", nidFuncCallSelfArgs };
  f( colon, a, e );

  // bnf::Nfunc lsb{ "prefixexp_index_lsb", llua::leftsquarebrackets_f, nidLeftSquareBrackets };
  // bnf::Nfunc rsb{ "prefixexp_index_rsb", llua::rightsquarebrackets_f, nidRightSquareBrackets };
  bnf::Tfunc<nid_leftsquarebrackets, nidLeftSquareBrackets> lsb{ "prefixexp_index_lsb" };
  bnf::Tfunc<nid_rightsquarebrackets, nidRightSquareBrackets> rsb{ "prefixexp_index_rsb" };
  bnf::Sseque b{ "prefixexp_indexexp", nidPrefixExpIndexExp };
  b( lsb, exp, rsb );

  // bnf::Nfunc dot{ "prefixexp_indexnamedot", llua::dot_f, nidDot };
  bnf::Tfunc<nid_dot, nidDot> dot{ "prefixexp_indexnamedot" };
  bnf::Sseque c{ "prefixexp_indexname", nidPrefixExpIndexName };
  c( dot, a );

  // bnf::Nfunc leftparenthesis{ "prefixexp_leftparenthesis", llua::leftparenthesis_f,
  // nidLeftParenthesis };
  // bnf::Nfunc rightparenthesis{ "prefixexp_rightparenthesis", llua::rightparenthesis_f,
  // nidRightParenthesis };
  bnf::Tfunc<nid_leftparenthesis, nidLeftParenthesis> leftparenthesis{
      "prefixexp_leftparenthesis" };
  bnf::Tfunc<nid_rightparenthesis, nidRightParenthesis> rightparenthesis{
      "prefixexp_rightparenthesis" };
  bnf::Sseque d{ "nidPrefixExpExpInParenthesis", nidPrefixExpExpInParenthesis };
  d( leftparenthesis, exp, rightparenthesis );

  bnf::Salter T{ "prefixexp_tail", nidPrefixExpTail, 0, -1 };
  T( e, f, b, c );

  bnf::Sseque aT{ "prefixexp_namebegin", nidPrefixExpNameBegin };
  aT( a, T );

  bnf::Sseque dT{ "prefixexp_expbegin", nidPrefixExpExpBegin };
  dT( d, T );

  prefixexp( aT, dT );
  bnf::pool_tree( prefixexp );
  assert( prefixexp.candidate_list->size() );
}

//----------------------------------------------------------------------------------------------
// a==Name
// b=='['exp']'
// c='.'Name
// d=='('exp')'
// e==args
// f==':'Name args
//
// I=>(e|f|c)I | bJ
// J=>(e|f|c)I | bJ|ε
// K=>(e|f|b)K | cL
// L=>(e|f|b)K | cL|ε

// V=>a | aI | dI | aK | dK
bnf::Salter &llua::create_var( bnf::Salter &exp, bnf::Salter &args ) {
  bnf::Salter astVar{ "var", nidVar };

  bnf::Tfunc<nid_colon, nidColon> colon{ "colon" };
  bnf::Tfunc<nid_name, nidName> a{ "var_name" };
  auto &e = args;

  bnf::Sseque f{ "var_funccallselfargs", nidFuncCallSelfArgs };
  f( colon, a, e );

  bnf::Tfunc<nid_leftsquarebrackets, nidLeftSquareBrackets> lsb{ "var_index_lsb" };
  bnf::Tfunc<nid_rightsquarebrackets, nidRightSquareBrackets> rsb{ "var_index_rsb" };
  bnf::Sseque b{ "var_indexexp", nidVarExpIndexExp };
  b( lsb, exp, rsb );

  bnf::Tfunc<nid_dot, nidDot> dot{ "var_indexnamedot" };
  bnf::Sseque c{ "var_indexname", nidVarExpIndexName };
  c( dot, a );

  bnf::Tfunc<nid_leftparenthesis, nidLeftParenthesis> leftparenthesis{ "var_leftparenthesis" };
  bnf::Tfunc<nid_rightparenthesis, nidRightParenthesis> rightparenthesis{ "var_rightparenthesis" };
  bnf::Sseque d{ "nidVarExpInParenthesis", nidVarExpInParenthesis };
  d( leftparenthesis, exp, rightparenthesis );

  bnf::Salter efc{ "var_callorindexbydot", nidVarCallOrIndexByDot };
  efc( e, f, c );

  bnf::Sseque efcI{ "var_postfix_callorindexbydot", nidVarPostfixCallOrIndexByDot };
  bnf::Salter I{ "var_postfix_callorindexbydot_pending", nidVarPostfixCallOrIndexByDotPending };
  efcI( efc, I );

  bnf::Sseque bJ{ "var_postfix_callorindexbydot_canbeempty_squareindex",
                  nidVarPostfixCallOrIndexByDotCanBeEmptySquareIndex };
  bnf::Salter J{ "var_postfix_callorindexbydot_canbeempty", nidVarPostfixCallOrIndexByDotCanBeEmpty,
                 0 };
  bJ( b, J );

  // I=>(e|f|c)I | bJ
  // J=>(e|f|c)I | bJ | ε
  I( efcI, bJ );
  J( efcI, bJ );

  bnf::Salter efb{ "var_callorindexbysquare", nidVarCallOrIndexBySquare };
  efb( e, f, b );

  bnf::Sseque efbK{ "var_postfix_callorindexbysquare" };
  bnf::Salter K{ "var_postfix_callorindexbysquare_pending",
                 nidVarPostfixCallOrIndexBySquarePending };
  efbK( efb, K );

  bnf::Sseque cL{ "var_postfix_callorindexbysquare_canbeempty_dotindex",
                  nidVarPostfixCallOrIndexBySquareCanBeEmptyDotIndex };
  bnf::Salter L{ "var_postfix_callorindexbysquare_canbeempty",
                 nidVarPostfixCallOrIndexBySquareCanBeEmpty, 0 };
  cL( c, L );

  // K=>(e|f|b)K | cL
  // L=>(e|f|b)K | cL|ε
  K( efbK, cL );
  L( efbK, cL );

  bnf::Sseque aI{ "var_prefix_name_callordotindex", nidVarPrefixNameCallOrDotIndex };
  bnf::Sseque dI{ "var_prefix_parenthesisexp_callordotindex",
                  nidVarPrefixParenthesisExpCallOrDotIndex };
  bnf::Sseque aK{ "var_prefix_name_callorsquareindex", nidVarPrefixNameCallOrSquareIndex };
  bnf::Sseque dK{ "var_prefix_parenthesisexp_callorsquareindex",
                  nidVarPrefixParenthesisExpCallOrSquareIndex };
  aI( a, I );
  dI( d, I );
  aK( a, K );
  dK( d, K );

  // V=>a | aI | dI | aK | dK
  astVar( aI, dI, aK, dK, a );
  auto pnVar = POOL_TREE( astVar );
  assert( pnVar );
  return ( *pnVar );
}

//----------------------------------------------------------------------------------------------
// funccall ::=  prefixexp args | prefixexp ‘:’ Name args
// a==Name
// b=='['exp']'
// c='.'Name
// d=='('exp')'
// e==args
// f==':'Name args
//
// I=>(b|c|f)I | eJ
// J=>(b|c|f)I | eJ | ε
// K=>(b|c|e)K | fL
// L=>(b|c|e)K | fL | ε
//
// X=>aI | dI | aK | dK
bnf::Salter &llua::create_funccall( bnf::Salter &exp, bnf::Salter &args ) {
  bnf::Salter astFuncCall{ "funccall", nidFuncCall };
  // bnf::Nfunc colon{ "colon", llua::colon_f, nidColon };
  // bnf::Nfunc a{ "funccall_name", llua::name_f, nidName };
  bnf::Tfunc<nid_colon, nidColon> colon{ "colon" };
  bnf::Tfunc<nid_name, nidName> a{ "funccall_name" };
  auto &e = args;

  bnf::Sseque f{ "funccall_funccallselfargs", nidFuncCallSelfArgs };
  f( colon, a, e );

  // bnf::Nfunc lsb{ "funccall_index_lsb", llua::leftsquarebrackets_f, nidLeftSquareBrackets };
  // bnf::Nfunc rsb{ "funccall_index_rsb", llua::rightsquarebrackets_f, nidRightSquareBrackets };
  bnf::Tfunc<nid_leftsquarebrackets, nidLeftSquareBrackets> lsb{ "funccall_index_lsb" };
  bnf::Tfunc<nid_rightsquarebrackets, nidRightSquareBrackets> rsb{ "funccall_index_rsb" };
  bnf::Sseque b{ "funccall_indexexp", nidFuncCallIndexExp };
  b( lsb, exp, rsb );

  // bnf::Nfunc dot{ "funccall_indexnamedot", llua::dot_f, nidDot };
  bnf::Tfunc<nid_dot, nidDot> dot{ "funccall_indexnamedot" };
  bnf::Sseque c{ "funccall_indexname", nidFuncCallIndexName };
  c( dot, a );

  // bnf::Nfunc leftparenthesis{ "funccall_leftparenthesis", llua::leftparenthesis_f,
  // nidLeftParenthesis };
  // bnf::Nfunc rightparenthesis{ "funccall_rightparenthesis", llua::rightparenthesis_f,
  // nidRightParenthesis };
  bnf::Tfunc<nid_leftparenthesis, nidLeftParenthesis> leftparenthesis{ "funccall_leftparenthesis" };
  bnf::Tfunc<nid_rightparenthesis, nidRightParenthesis> rightparenthesis{
      "funccall_rightparenthesis" };
  bnf::Sseque d{ "funccall_parenthesisexp", nidFuncCallParenthesisExp };
  d( leftparenthesis, exp, rightparenthesis );

  bnf::Salter bcf{ "funccall_callorindex", nidFuncCallSelfCallOrIndex };
  bcf( b, c, f );

  bnf::Sseque bcfI{ "funccall_postfix_selfcallorindex", nidFuncCallPostfixSelfCallOrIndex };
  bnf::Salter I{ "funccall_postfix_selfcallorindex_pending",
                 nidFuncCallPostfixSelfCallOrIndexPending };
  bcfI( bcf, I );

  bnf::Sseque eJ{ "funccall_postfix_selfcallorindex_canbeempty_args",
                  nidFuncCallPostfixSelfCallOrIndexCanBeEmptyArgs };
  bnf::Salter J{ "funccall_postfix_selfcallorindex_canbeempty",
                 nidFuncCallPostfixSelfCallOrIndexCanBeEmpty, 0 };
  eJ( e, J );

  // I=>(b|c|f)I | eJ
  // J=>(b|c|f)I | eJ | ε
  I( bcfI, eJ );
  J( bcfI, eJ );

  bnf::Salter bce{ "funccall_callorindex", nidFuncCallCallOrIndex };
  bce( b, c, e );

  bnf::Sseque bceK{ "funccall_postfix_callorindex" };
  bnf::Salter K{ "funccall_postfix_callorindex_pending", nidFuncCallPostfixCallOrIndexPending };
  bceK( bce, K );

  bnf::Sseque fL{ "funccall_postfix_callorindex_canbeempty_selfcall",
                  nidFuncCallPostfixCallOrIndexCanBeEmptySelfCall };
  bnf::Salter L{ "funccall_postfix_callorindex_canbeempty", nidFuncCallPostfixCallOrIndexCanBeEmpty,
                 0 };
  fL( f, L );

  // K=>(b|c|e)K | fL
  // L=>(b|c|e)K | fL | ε
  K( bceK, fL );
  L( bceK, fL );

  bnf::Sseque aI{ "funccall_prefix_name_callordotindex", nidVarPrefixNameCallOrDotIndex };
  bnf::Sseque dI{ "funccall_prefix_parenthesisexp_callordotindex",
                  nidVarPrefixParenthesisExpCallOrDotIndex };
  bnf::Sseque aK{ "funccall_prefix_name_callorsquareindex", nidVarPrefixNameCallOrSquareIndex };
  bnf::Sseque dK{ "funccall_prefix_parenthesisexp_callorsquareindex",
                  nidVarPrefixParenthesisExpCallOrSquareIndex };
  // X=>aI | dI | aK | dK
  aI( a, I );
  dI( d, I );
  aK( a, K );
  dK( d, K );
  astFuncCall( aI, dI, aK, dK );
  auto pnFuncCall = POOL_TREE( astFuncCall );
  assert( pnFuncCall );
  return ( *pnFuncCall );
}

//----------------------------------------------------------------------------------------------
// do block end
bnf::Sseque &llua::create_doend( bnf::Sseque &block ) {
  bnf::Sseque astDoEnd{ "doend", nidDoEnd };
  // bnf::Nfunc do_{ "do", llua::do_f, nidDo };
  // bnf::Nfunc end{ "end", llua::end_f, nidEnd };
  bnf::Tfunc<nid_kwdo, nidDo> do_{ "do" };
  bnf::Tfunc<nid_kwend, nidEnd> end{ "end" };
  astDoEnd( do_, block, end );
  auto pnDoEnd = POOL_TREE( astDoEnd );
  assert( pnDoEnd );
  return ( *pnDoEnd );
}

//----------------------------------------------------------------------------------------------
// local namelist [‘=’ explist]
bnf::Sseque &llua::create_localnamelist( bnf::Sseque &namelist, bnf::Sseque &explist ) {
  bnf::Sseque astLocalNameList{ "localnamelist", nidLocalNameList };
  // bnf::Nfunc local{ "local", llua::local_f, nidLocal };
  // bnf::Nfunc assign{ "assign", llua::assign_f, nidAssign };
  bnf::Tfunc<nid_kwlocal, nidLocal> local{ "local" };
  bnf::Tfunc<nid_assign, nidAssign> assign{ "assign" };
  bnf::Sseque localnamelist_option{ "localnamelist_option", nidLocalNameListOption, 0 };
  localnamelist_option( assign, explist );

  astLocalNameList( local, namelist, localnamelist_option );
  auto pnLocalNameList = POOL_TREE( astLocalNameList );
  assert( pnLocalNameList );
  return ( *pnLocalNameList );
}

//----------------------------------------------------------------------------------------------
// args ::=  ‘(’ [explist] ‘)’ | tableconstructor | String
bnf::Salter &llua::create_args( bnf::Sseque &explist, bnf::Sseque &tableconsntructor ) {
  bnf::Nfunc astString{ "string", string_f, string_frs, nidString };
  bnf::Salter astArgs{ "args", nidArgs };
  bnf::Sseque args_explist{ "args_explist", nidArgsExplist };
  bnf::Sseque args_explist_optionexplist{ "args_explist_optionexplist", nidArgsExplistOptionExplist,
                                          0 };
  args_explist_optionexplist( explist );

  // bnf::Nfunc leftparenthesis{ "args_leftparenthesis", llua::leftparenthesis_f, nidLeftParenthesis
  // }; bnf::Nfunc rightparenthesis{ "args_rightparenthesis", llua::rightparenthesis_f,
  // nidRightParenthesis };
  bnf::Tfunc<nid_leftparenthesis, nidLeftParenthesis> leftparenthesis{ "args_leftparenthesis" };
  bnf::Tfunc<nid_rightparenthesis, nidRightParenthesis> rightparenthesis{ "args_rightparenthesis" };
  args_explist( leftparenthesis, args_explist_optionexplist, rightparenthesis );

  astArgs( args_explist, tableconsntructor, astString );
  auto pnArgs = POOL_TREE( astArgs );
  assert( pnArgs );
  return ( *pnArgs );
}

//----------------------------------------------------------------------------------------------
// functiondef ::= function funcbody
bnf::Sseque &llua::create_functiondef( bnf::Sseque &funcbody ) {
  bnf::Sseque astFunctionDef{ "functiondef", nidFunctionDef };
  // bnf::Nfunc kwfunction{ "function", llua::function_f, nidFunction };
  bnf::Tfunc<nid_kwfunction, nidFunction> kwfunction{ "function" };
  astFunctionDef( kwfunction, funcbody );
  auto pnFunctionDef = POOL_TREE( astFunctionDef );
  assert( pnFunctionDef );
  return ( *pnFunctionDef );
}

//----------------------------------------------------------------------------------------------
// funcbody ::= ‘(’ [parlist] ‘)’ block end
bnf::Sseque &llua::create_funcbody( bnf::Salter &parlist, bnf::Sseque &block ) {
  bnf::Sseque astFuncBody{ "funcbody", nidFuncBody };
  // bnf::Nfunc leftparenthesis{ "leftparenthesis", llua::leftparenthesis_f, nidLeftParenthesis };
  // bnf::Nfunc rightparenthesis{ "rightparenthesis", llua::rightparenthesis_f, nidRightParenthesis
  // };
  bnf::Tfunc<nid_leftparenthesis, nidLeftSquareBrackets> leftparenthesis{ "leftparenthesis" };
  bnf::Tfunc<nid_rightparenthesis, nidRightParenthesis> rightparenthesis{ "rightparenthesis" };
  bnf::Sseque optionparlist{ "funcbody_optionparlist", nidFuncBodyOptionParList, 0, 1 };
  optionparlist( parlist );
  // bnf::Nfunc end{ "end", llua::end_f, nidEnd };
  bnf::Tfunc<nid_kwend, nidEnd> end{ "end" };
  astFuncBody( leftparenthesis, optionparlist, rightparenthesis, block, end );
  auto pnFuncBody = POOL_TREE( astFuncBody );
  assert( pnFuncBody );
  return ( *pnFuncBody );
}

//----------------------------------------------------------------------------------------------
int llua::parse_ast( bnf::Source_token &source, bnf::Stack<Token> &stack, ::Result &result,
                     bnf::Parser<Token> &parser ) {
  int ret = roadmap::fail;
  // XXX: node is epsilon when rang_min==0

  // field
  // exp
  bnf::Salter *pnExp = nullptr;
  auto astField = create_field( &pnExp );
  auto &astExp = *pnExp;

  // funcname
  auto &astFuncName = create_funcname();

  // fieldlist
  auto astFieldList = create_fieldlist( astField );

  // tableconstructor
  auto astTableConstructor = create_tableconstructor( astFieldList );

  // label
  bnf::Nfunc astLabel{ "label", label_f, label_frs, AST_ID::nidLabel };

  // namelist
  auto astNameList = create_namelist();

  // parlist
  auto astParList = create_parlist( astNameList );

  // explist
  auto astExpList = create_explist( astExp );

  // retstat
  auto astRetstat = create_retstat( astExpList );

  // block
  // stat
  bnf::Salter *pnStat = nullptr;
  auto astBlock = create_block( &pnStat, astRetstat );
  auto &astStat = *pnStat;

  // do block end
  auto astDoEnd = create_doend( astBlock );

  // funcbody
  auto astFuncBody = create_funcbody( astParList, astBlock );

  // functiondef
  auto astFunctionDef = create_functiondef( astFuncBody );

  // args
  auto astArgs = create_args( astExpList, astTableConstructor );

  // exp
  // prefixexp
  bnf::Salter *pnPrefixExp = nullptr;
  setup_exp( astExp, astFunctionDef, &pnPrefixExp, astTableConstructor );
  auto &astPrefixExp = *pnPrefixExp;
  setup_prefixexp( astPrefixExp, astExp, astArgs );

  // var
  auto astVar = create_var( astExp, astArgs );

  // varlist
  auto astVarList = create_varlist( astVar );

  // varlist = explist
  auto astVarListAssign = create_varlistassign( astVarList, astExpList );

  // funccall
  auto astFuncCall = create_funccall( astExp, astArgs );

  // break
  // bnf::Sfunc astBreak{ "break", break_f, nidBreak };
  bnf::Tfunc<nid_kwbreak, nidBreak> astBreak{ "break" };

  // break
  // bnf::Sfunc astSemiColon{ "semicolon", semicolon_f, nidSemiColon };
  bnf::Tfunc<nid_semicolon, nidSemiColon> astSemiColon{ "semicolon" };

  // goto Name
  auto astGotoName = create_gotoname();

  // while exp do block end
  auto astWhileDo = create_whiledo( astExp, astBlock );

  // repeat block until exp
  auto astRepeatUntil = create_repeatuntil( astBlock, astExp );

  // if exp then block {elseif exp then block} [else block] end |
  auto astIfThen = create_ifthen( astBlock, astExp );

  // for Name ‘=’ exp ‘,’ exp [‘,’ exp] do block end |
  auto astForDo = create_fordo( astExp, astBlock );

  // for namelist in explist do block end
  auto astForIn = create_forin( astNameList, astExpList, astBlock );

  // function funcname funcbody
  auto astFunctionDefine = create_functiondefine( astFuncName, astFuncBody );

  // local function Name funcbody
  auto astLocalFuncDefine = create_localfuncdefine( astFuncBody );

  // local namelist [‘=’ explist]
  auto astLocalNameList = create_localnamelist( astNameList, astExpList );

  // stat
  astStat.r_conf.hierarchy = false;
  astStat( astSemiColon, astLabel, astDoEnd, astLocalNameList, astFuncCall, astBreak, astWhileDo,
           astVarListAssign, astRepeatUntil, astIfThen, astForDo, astForIn, astFunctionDefine,
           astLocalFuncDefine );

  // chunk
  bnf::Salter chunk{ "chunk", nidChunk };
  chunk( astBlock );
  auto G = bnf::pool_tree( chunk );

  ret = parser.parse( G );

  return ret;
}

//----------------------------------------------------------------------------------------------
void llua::filter_space_comment( bnf::Result &result, bnf::tokens_t &dest ) {
  for ( auto &i : result.data ) {
    if ( NODE_ID::nid_S != i.nid && NODE_ID::nid_comment != i.nid &&
         result_type::match_content == i.type ) {
      dest.push_back( { i.nid, i.bi, i.ei } );
    }
  }
}

//----------------------------------------------------------------------------------------------
int llua::lex_markkeyword( bnf::Source<uchar> &src, bnf::Result &result ) {
  for ( auto &i : result.data ) {
    if ( nid_name == i.nid ) { // identifier
      auto len = i.ei.index - i.bi.index;
      switch ( len ) {
        case 2: // do or if in
          if ( 0 == src.compare( i.bi, cdo, len ) ) {
            i.nid = nid_kwdo;
            i.name = cdo;
          } else if ( 0 == src.compare( i.bi, cif, len ) ) {
            i.nid = nid_kwif;
            i.name = cif;
          } else if ( 0 == src.compare( i.bi, cin, len ) ) {
            i.nid = nid_kwin;
            i.name = cin;
          } else if ( 0 == src.compare( i.bi, cor, len ) ) {
            i.nid = nid_kwor;
            i.name = cor;
          }
          break;
        case 3: // and end for nil not
          if ( 0 == src.compare( i.bi, cand, len ) ) {
            i.nid = nid_kwand;
            i.name = cand;
          } else if ( 0 == src.compare( i.bi, cend, len ) ) {
            i.nid = nid_kwend;
            i.name = cend;
          } else if ( 0 == src.compare( i.bi, cfor, len ) ) {
            i.nid = nid_kwfor;
            i.name = cfor;
          } else if ( 0 == src.compare( i.bi, cnil, len ) ) {
            i.nid = nid_kwnil;
            i.name = cnil;
          } else if ( 0 == src.compare( i.bi, cnot, len ) ) {
            i.nid = nid_kwnot;
            i.name = cnot;
          }
          break;
        case 4: // else goto then true
          if ( 0 == src.compare( i.bi, celse, len ) ) {
            i.nid = nid_kwelse;
            i.name = celse;
          } else if ( 0 == src.compare( i.bi, cgoto, len ) ) {
            i.nid = nid_kwgoto;
            i.name = cgoto;
          } else if ( 0 == src.compare( i.bi, cthen, len ) ) {
            i.nid = nid_kwthen;
            i.name = cthen;
          } else if ( 0 == src.compare( i.bi, ctrue, len ) ) {
            i.nid = nid_kwtrue;
            i.name = ctrue;
          }
          break;
        case 5: // break false local until while
          if ( 0 == src.compare( i.bi, cbreak, len ) ) {
            i.nid = nid_kwbreak;
            i.name = cbreak;
          } else if ( 0 == src.compare( i.bi, cfalse, len ) ) {
            i.nid = nid_kwfalse;
            i.name = cfalse;
          } else if ( 0 == src.compare( i.bi, clocal, len ) ) {
            i.nid = nid_kwlocal;
            i.name = clocal;
          } else if ( 0 == src.compare( i.bi, cuntil, len ) ) {
            i.nid = nid_kwuntil;
            i.name = cuntil;
          } else if ( 0 == src.compare( i.bi, cwhile, len ) ) {
            i.nid = nid_kwwhile;
            i.name = cwhile;
          }
          break;
        case 6: // elseif repeat return
          if ( 0 == src.compare( i.bi, celseif, len ) ) {
            i.nid = nid_kwelseif;
            i.name = celseif;
          } else if ( 0 == src.compare( i.bi, crepeat, len ) ) {
            i.nid = nid_kwrepeat;
            i.name = crepeat;
          } else if ( 0 == src.compare( i.bi, creturn, len ) ) {
            i.nid = nid_kwreturn;
            i.name = creturn;
          }
          break;
        case 8: // function
          if ( 0 == src.compare( i.bi, cfunction, len ) ) {
            i.nid = nid_kwfunction;
            i.name = cfunction;
          }
          break;
      }
    }
  }
  return 0;
}

//----------------------------------------------------------------------------------------------
roadmap llua::OPERATOR_FRS( const Symbol &symbol ) {
  roadmap rdm = roadmap::unmatched;
  switch ( symbol.ch ) {
    case '=':
    case '~':
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '.':
    case ';':
    case ':':
    case '&':
    case '|':
    case '>':
    case '<':
    case '^':
    case ',':
    case '[':
    case ']':
    case '(':
    case ')':
    case '{':
    case '}':
    case '#': rdm = roadmap::matched;
  }
  return rdm;
}

roadmap llua::OPERATOR_F( Nfunc<uchar> &node ) {
  auto rdm = roadmap::unmatched;

  enum class S : i32 {
    S0 = 0, // 开始状态
    ASN,
    BNT,
    DOT,
    GRT,
    LES,
    JON,
    COL,
    DIV,
    LAST,
    ERR = 0x10,
    SUC,
    EBD = 0x1000, // 当前输入不属于匹配范围，已匹配的可以构成一个整体
    IDN = 0x100,  // 以下状态均为同为一种转移模式
    UEQ,
    GRE,
    LEE,
    BRG,
    BLF,
    ADD,
    SUB,
    MUL,
    MOD,
    SEM,
    AND,
    BOR,
    POW,
    COM,
    LSB,
    RSB,
    LPT,
    RPT,
    LBR,
    RBR,
    SHA,
    ELL,
    DCL,
    FDV, // floor divide
  } state{ S::S0 };

  enum class I : i32 {
    asn = 0, //=
    bnt,     //~
    add,     //+
    sub,     //-
    mul,     // *
    div,     ///
    mod,     //%
    dot,     //.
    sem,     //;
    col,     //:
    and_,    //&
    bor,     //|
    grt,     //>
    les,     //<
    pow,     //^
    com,     //,
    lsb,     //[
    rsb,     //]
    lpt,     //(
    rpt,     //)
    lbr,     //{
    rbr,     //}
    sha,     //#
    rest,    // 除以上面的符号
    LAST,
  } input{ I::rest };

  // 边界符号不应包括进匹配范围
  // 该匹配没有明确边界符号，需要外部符号来帮助判断是否匹配终止，实际上SUC==EBD
  S N[(i32)S::LAST][(i32)I::LAST] = {
      { S::ASN, S::BNT, S::ADD, S::SUB, S::MUL, S::DIV, S::MOD, S::DOT,
        S::SEM, S::COL, S::AND, S::BOR, S::GRT, S::LES, S::POW, S::COM,
        S::LSB, S::RSB, S::LPT, S::RPT, S::LBR, S::RBR, S::SHA, S::ERR }, // S0
      { S::IDN, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD }, // ASN
      { S::UEQ, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD }, // BNT
      { S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::JON,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD }, // DOT
      { S::GRE, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::JON,
        S::EBD, S::EBD, S::EBD, S::EBD, S::BRG, S::EBD, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD }, // GRT
      { S::LEE, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::JON,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::BLF, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD }, // LES
      { S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::ELL,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD }, // JON
      { S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD,
        S::EBD, S::DCL, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD }, // COL
      { S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::FDV, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD,
        S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD, S::EBD }, // FDV
  };

  auto mapinput = []( uchar ch ) -> I {
    // regard eof as rest
    auto ret = I::rest;
    switch ( ch ) {
      case '=': ret = I::asn; break;
      case '~': ret = I::bnt; break; // bit xor
      case '+': ret = I::add; break;
      case '-': ret = I::sub; break;
      case '*': ret = I::mul; break;
      case '/': ret = I::div; break;
      case '%': ret = I::mod; break;
      case '.': ret = I::dot; break;
      case ';': ret = I::sem; break;
      case ':': ret = I::col; break;
      case '&': ret = I::and_; break;
      case '|': ret = I::bor; break;
      case '>': ret = I::grt; break;
      case '<': ret = I::les; break;
      case '^': ret = I::pow; break;
      case ',': ret = I::com; break;
      case '[': ret = I::lsb; break;
      case ']': ret = I::rsb; break;
      case '(': ret = I::lpt; break;
      case ')': ret = I::rpt; break;
      case '{': ret = I::lbr; break;
      case '}': ret = I::rbr; break;
      case '#': ret = I::sha; break;
      default: ret = I::rest;
    }
    return ret;
  };

  auto ok = true;
  S last_if_ebd = S::S0;
  auto &source = *node.pSource;
  do {
    uchar ch = '\0'; // 当eof时，输入的值
    ok = source.get( ch );
    input = mapinput( ch );
    if ( state < S::LAST ) {
      auto temp = N[(i32)state][(i32)input];
      if ( temp == S::EBD )
        last_if_ebd = state;
      state = temp;
    } else {
      if ( state >= S::IDN ) {
        // state = ( input == I::rest ) ? ( last_if_ebd = state, S::EBD ) : S::ERR;
        last_if_ebd = state;
        state = S::EBD;
      }
    }
    if ( ( state != S::ERR ) && ( state != S::SUC ) && ( state != S::EBD ) ) {
      source.getposition( node.match_ei );
    } else {
      break;
    }
  } while ( ok );

  rdm = ( state == S::SUC || state == S::EBD ) ? roadmap::matched : roadmap::unmatched;
  if ( roadmap::matched == rdm ) {
    if ( state == S::EBD ) {
      source.setposition( node.match_ei ); // 将位置移回匹配范围(放回多读取的边界字符)
    }
    NODE_ID nid = nid_operator;
    bnf::cch *n = nullptr;
    switch ( last_if_ebd ) {
      case S::ASN:
        nid = nid_assign;
        n = "=";
        break;
      case S::BNT:
        nid = nid_bitnot;
        n = "~";
        break;
      case S::DOT:
        nid = nid_dot;
        n = ".";
        break;
      case S::GRT:
        nid = nid_great;
        n = ">";
        break;
      case S::LES:
        nid = nid_less;
        n = "<";
        break;
      case S::IDN:
        nid = nid_identity;
        n = "==";
        break;
      case S::UEQ:
        nid = nid_notidentity;
        n = "~=";
        break;
      case S::GRE:
        nid = nid_greatequal;
        n = ">=";
        break;
      case S::LEE:
        nid = nid_lessequal;
        n = "<=";
        break;
      case S::JON:
        nid = nid_joining;
        n = "..";
        break;
      case S::BRG:
        nid = nid_bitright;
        n = ">>";
        break;
      case S::BLF:
        nid = nid_bitleft;
        n = "<<";
        break;
      case S::ADD:
        nid = nid_plus;
        n = "+";
        break;
      case S::SUB:
        nid = nid_subtract;
        n = "-";
        break;
      case S::MUL:
        nid = nid_multiply;
        n = "*";
        break;
      case S::DIV:
        nid = nid_divide;
        n = "/";
        break;
      case S::MOD:
        nid = nid_mod;
        n = "%";
        break;
      case S::SEM:
        nid = nid_semicolon;
        n = ";";
        break;
      case S::COL:
        nid = nid_colon;
        n = ":";
        break;
      case S::FDV:
        nid = nid_floordivide;
        n = "//";
        break;
      case S::AND:
        nid = nid_bitand;
        n = "&";
        break;
      case S::BOR:
        nid = nid_bitor;
        n = "|";
        break;
      case S::POW:
        nid = nid_power;
        n = "^";
        break;
      case S::COM:
        nid = nid_comma;
        n = ",";
        break;
      case S::LSB:
        nid = nid_leftsquarebrackets;
        n = "[";
        break;
      case S::RSB:
        nid = nid_rightsquarebrackets;
        n = "]";
        break;
      case S::LPT:
        nid = nid_leftparenthesis;
        n = "(";
        break;
      case S::RPT:
        nid = nid_rightparenthesis;
        n = ")";
        break;
      case S::LBR:
        nid = nid_leftbrace;
        n = "{";
        break;
      case S::RBR:
        nid = nid_rightbrace;
        n = "}";
        break;
      case S::SHA:
        nid = nid_sharp;
        n = "#";
        break;
      case S::ELL:
        nid = nid_ellipse;
        n = "...";
        break;
      case S::DCL:
        nid = nid_doublecolon;
        n = "::";
        break;
    }
    node.node_id = nid;
    if ( n ) {
      node.name = n;
    }
  } else {
    source.setposition( node.match_bi );
  }

  return rdm;
}

//----------------------------------------------------------------------------------------------
roadmap llua::Comment_FRS( const Symbol &symbol ) {
  return ( '-' == symbol.ch ) ? roadmap::matched : roadmap::unmatched;
}

roadmap llua::Comment_F( Nfunc<uchar> &node ) {
  auto rdm = roadmap::unmatched;

  enum class S : i32 {
    S0 = 0, // 状态行，从0开始,枚举值表示状态表的行的下标
    MNU1,   // 第一个减号
    MNU2,   // 第二个减号
    CMT1,   // 普通注释
    LSB1,   // 第一个左方括号
    CTN1,   // 内容部分(无等号分隔方式)
    RSB1,   // 无等号分隔的第一个右方括号
    CTP1,   // 左分隔等号
    CTN2,   // 内容部分(有等号分隔)
    RSB2,   // 有等号分隔的第一个右方括号
    CTP2,   // 右分隔等号
    LAST,   // 状态数量标志
    ERR,
    SUC,
    MSB = 0x10000, // Mix State Base
    CTP1$count,
    CTN2$acmount,
    CTP2$sym1,
    CTP2$CTN2$sym2,
    SUC$CTN2$issym,
  } state{ S::S0 };

  enum class I : i32 {
    minus, // 减号
    lsb,   // 左方括号
    rsb,   // 右方括号
    equal, // 等号
    crlf,  // 回车
    eof,   // 输入结束
    rest,  // 其余
    LAST,  // 标志
  } input{ I::rest };

  //  minus,lsb,rsb,equal,crlf,eof,rest
  S N[(i32)S::LAST][(i32)I::LAST] = {
      { S::MNU1, S::ERR, S::ERR, S::ERR, S::ERR, S::ERR, S::ERR },                          // S0
      { S::MNU2, S::ERR, S::ERR, S::ERR, S::ERR, S::ERR, S::ERR },                          // MNU1
      { S::CMT1, S::LSB1, S::CMT1, S::CMT1, S::SUC, S::SUC, S::CMT1 },                      // MNU2
      { S::CMT1, S::CMT1, S::CMT1, S::CMT1, S::SUC, S::SUC, S::CMT1 },                      // CMT1
      { S::CMT1, S::CTN1, S::CMT1, S::CTP1$count, S::SUC, S::SUC, S::CMT1 },                // LSB1
      { S::CTN1, S::CTN1, S::RSB1, S::CTN1, S::CTN1, S::ERR, S::CTN1 },                     // CTN1
      { S::CTN1, S::CTN1, S::SUC, S::CTN1, S::CTN1, S::ERR, S::CTN1 },                      // RSB1
      { S::CMT1, S::CTN2$acmount, S::CMT1, S::CTP1$count, S::SUC, S::SUC, S::CMT1 },        // CTP1
      { S::CTN2, S::CTN2, S::RSB2, S::CTN2, S::CTN2, S::ERR, S::CTN2 },                     // CTN2
      { S::CTN2, S::CTN2, S::SUC$CTN2$issym, S::CTP2$sym1, S::CTN2, S::ERR, S::CTN2 },      // RSB2
      { S::CTN2, S::CTN2, S::SUC$CTN2$issym, S::CTP2$CTN2$sym2, S::CTN2, S::ERR, S::CTN2 }, // CTP2
  };

  struct SMD { // state middle data
    int equal_count = 0;
    int equal_amount = 0;
    int equal_sym = 0;
  } smd;

  auto mapinput = []( uchar ch ) -> I {
    auto ret = I::rest;
    if ( ch == '-' )
      ret = I::minus;
    else if ( ch == '[' )
      ret = I::lsb;
    else if ( ch == ']' )
      ret = I::rsb;
    else if ( ch == '=' )
      ret = I::equal;
    else if ( ch == '\r' || ch == '\n' )
      ret = I::crlf;
    return ret;
  };

  bool ok = true;
  auto msb = static_cast<std::underlying_type_t<S>>( S::MSB );
  auto &source = *node.pSource;
  do {
    uchar ch = '\0';
    ok = source.get( ch );
    input = mapinput( ch );
    if ( !ok ) {
      input = I::eof;
    }
    state = N[(i32)state][(i32)input];
    auto s = static_cast<std::underlying_type_t<S>>( state );
    if ( s & msb ) {
      switch ( state ) {
        case S::CTP1$count:
          smd.equal_count += 1;
          state = S::CTP1;
          break;
        case S::CTN2$acmount:
          smd.equal_amount = smd.equal_count;
          state = S::CTN2;
          break;
        case S::CTP2$sym1:
          smd.equal_sym = smd.equal_amount;
          --smd.equal_sym;
          state = S::CTP2;
          break;
        case S::CTP2$CTN2$sym2:
          --smd.equal_sym;
          state = smd.equal_sym >= 0 ? S::CTP2 : S::CTN2;
          break;
        case S::SUC$CTN2$issym: state = smd.equal_sym == 0 ? S::SUC : S::CTN2;
      }
    }
    ////dbg( ch, input, state );

    if ( ( state != S::ERR ) && ( state != S::SUC ) ) {
      source.getposition( node.match_ei );
    } else {
      break;
    }
  } while ( ok );

  if ( state == S::SUC ) {
    rdm = roadmap::matched;
  } else {
    rdm = roadmap::unmatched;
    source.setposition( node.match_bi );
  }

  return rdm;
}

//----------------------------------------------------------------------------------------------
roadmap llua::String_FRS( const Symbol &symbol ) {
  return ( '[' == symbol.ch ) ? roadmap::matched : roadmap::unmatched;
}

// 接受[[...]] 或[==[...]==] 这一类字符串
roadmap llua::String_F( Nfunc<uchar> &node ) {
  auto rdm = roadmap::unmatched;

  // 状态
  enum class S : i32 {
    S0 = 0, // 状态行，从0开始,枚举值表示状态表的行的下标
    LSB1,   // 第一个左方括号
    CTN1,   // 内容部分(无等号分隔方式)
    RSB1,   // 无等号分隔的第一个右方括号
    CTP1,   // 左分隔等号
    CTN2,   // 内容部分(有等号分隔)
    RSB2,   // 有等号分隔的第一个右方括号
    CTP2,   // 右分隔等号
    LAST,   // 状态数量标志
    ERR,
    SUC,
    MSB = 0x10000, // Mix State Base
    CTP1$count,
    CTN2$acmount,
    CTP2$sym1,
    CTP2$CTN2$sym2,
    SUC$CTN2$issym,
  } state{ S::S0 };

  // 输入
  enum class I : i32 {
    lsb,   // 左方括号
    rsb,   // 右方括号
    equal, // 等号
    rest,  // 其余
    LAST,  // 标志
  } input{ I::lsb };

  //  lsb,rsb,equal,rest
  S N[(i32)S::LAST][(i32)I::LAST] = {
      { S::LSB1, S::ERR, S::ERR, S::ERR },                        // S0
      { S::CTN1, S::ERR, S::CTP1$count, S::ERR },                 // LSB1
      { S::CTN1, S::RSB1, S::CTN1, S::CTN1 },                     // CTN1
      { S::CTN1, S::SUC, S::CTN1, S::CTN1 },                      // RSB1
      { S::CTN2$acmount, S::ERR, S::CTP1$count, S::ERR },         // CTP1
      { S::CTN2, S::RSB2, S::CTN2, S::CTN2 },                     // CTN2
      { S::CTN2, S::SUC$CTN2$issym, S::CTP2$sym1, S::CTN2 },      // RSB2
      { S::CTN2, S::SUC$CTN2$issym, S::CTP2$CTN2$sym2, S::CTN2 }, // CTP2
  };

  struct SMD {
    int equal_count = 0;
    int equal_amount = 0;
    int equal_sym = 0;
  } smd;

  auto mapinput = []( uchar ch ) -> I {
    auto ret = I::rest;
    if ( ch == '[' )
      ret = I::lsb;
    else if ( ch == ']' )
      ret = I::rsb;
    else if ( ch == '=' )
      ret = I::equal;
    return ret;
  };

  bool ok = true;
  auto msb = static_cast<std::underlying_type_t<S>>( S::MSB );
  auto &source = *node.pSource;
  do {
    uchar ch = '\0';
    ok = source.get( ch );
    input = mapinput( ch );
    state = N[(i32)state][(i32)input];
    auto s = static_cast<std::underlying_type_t<S>>( state );
    if ( s & msb ) {
      switch ( state ) {
        case S::CTP1$count:
          smd.equal_count += 1;
          state = S::CTP1;
          break;
        case S::CTN2$acmount:
          smd.equal_amount = smd.equal_count;
          state = S::CTN2;
          break;
        case S::CTP2$sym1:
          smd.equal_sym = smd.equal_amount;
          --smd.equal_sym;
          state = S::CTP2;
          break;
        case S::CTP2$CTN2$sym2:
          --smd.equal_sym;
          state = smd.equal_sym >= 0 ? S::CTP2 : S::CTN2;
          break;
        case S::SUC$CTN2$issym: state = smd.equal_sym == 0 ? S::SUC : S::CTN2;
      }
    }

    if ( ( state != S::ERR ) && ( state != S::SUC ) ) {
      source.getposition( node.match_ei );
    } else {
      break;
    }
  } while ( ok );

  if ( state == S::SUC ) {
    rdm = roadmap::matched;
  } else {
    rdm = roadmap::unmatched;
    source.setposition( node.match_bi );
  }

  return rdm;

} // String_F
