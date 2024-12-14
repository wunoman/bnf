#include "bnf.h"
using namespace bnf;

//----------------------------------------------------------------------------------------------
Pool<Range> bnf::rangepool;
//----------------------------------------------------------------------------------------------
void bnf::print_tokens( const tokens_t &tks ) {
  printf( R"(
--tokens.size=%llu
tokens={
)", tks.size() );

  size_t index = 0;
  for ( auto &tk : tks ) {
    printf( "{%d, %lld, %lld}, --%llu\n", tk.nid, tk.bi.index, tk.ei.index, ++index );
  }
  printf( R"(
};
)" );
  fflush(stdout);
}

//----------------------------------------------------------------------------------------------
bool bnf::operator<( const Token &l, const Token &r ) {
  return ( l.nid < r.nid );
}

bool bnf::operator<( const Position &l, const Position &r ) {
  return ( l.index < r.index );
}
//----------------------------------------------------------------------------------------------
roadmap ANYDISTINCTSTR_F( Nfunc<uchar> &node ) {
  // 没有共同前缀的一组字符串,根据第一个字符做排除,只留下命中的一个做全串匹配
  auto rdm = roadmap::unmatched;
  auto &vs = *reinterpret_cast<Vpchs *>( node.data );

  auto matched_ch_count = 0;
  uchar ch = '\0';
  auto eof = node.pSource->get( ch );
  if ( eof )
    return rdm;
  ++matched_ch_count;
  auto index = 0;
  int size = vs.size();
  while ( ( index < size ) && ( *vs[index] != ch ) ) {
    ++index;
  }
  if ( index < size ) {
    auto pch = vs[index];
    do {
      eof = node.pSource->get( ch );
      ++matched_ch_count;
      ++pch;
    } while ( ( *pch == ch ) && ( *pch != '\0' ) && ( !eof ) );
    if ( *pch == '\0' ) {
      node.match_ei.index += matched_ch_count - 1;
      node.pSource->setposition( node.match_ei );
      rdm = roadmap::matched;
    } else {
      node.pSource->setposition( node.match_bi );
      rdm = roadmap::unmatched;
    }
  }
  return rdm;
}

//----------------------------------------------------------------------------------------------
Range *Range::create_header( Range::Pool_t &pool ) {
  auto rng = pool.clone( {} );
  rng->head = rng->last = rng;
  return rng;
}

Range::item_t &Range::get_first() {
  assert( head && head->arr_index >= 0 );
  return head->arr[0];
}

Range::item_t &Range::get_last() {
  assert( last && last->arr_index >= 0 && last->arr_index < ARRSIZE );
  return last->arr[last->arr_index];
}

void Range::pop_back() {
  assert( last && last->arr_index >= 0 && last->arr_index < ARRSIZE );
  if ( last->arr_index == 0 ) {
    if ( last != head ) {
      auto new_last = last->prev;
      assert( new_last );
      new_last->next = nullptr;
      reinterpret_cast<Range::Pool_t *>( pool_object )->release( last );
      last = new_last;
    } else {
      assert( next == nullptr );
      last->arr_index = Range::ARRINDEXDEFAULT;
    }
  } else {
    --last->arr_index;
  }
}

void Range::push_state( const item_t &item ) {
  assert( last && last->arr_index < ARRSIZE );
  if ( last->arr_index == MAXARRINDEX ) {
    auto rng = reinterpret_cast<Range::Pool_t *>( pool_object )->clone( {} );
    assert( rng && rng->arr_index == Range::ARRINDEXDEFAULT );
    rng->prev = last;
    last->next = rng;
    last = rng;
    rng->head = head;
  } else {
    assert( last->arr_index >= Range::ARRINDEXDEFAULT && last->arr_index < MAXARRINDEX );
  }
  last->arr[++last->arr_index] = item;
}

void Range::release() {
  auto p = this;
  Range *n = nullptr;
  auto pool = reinterpret_cast<Range::Pool_t *>( pool_object );
  while ( p ) {
    n = p->next;
    pool->release( p );
    p = n;
  }
}

//----------------------------------------------------------------------------------------------
Source_str::Source_str( cch *pch, intptr_t len ) : str( pch ) {
  assert( pch );
  length = ( len == -1 ) ? str.length() : len;
}

// return false if eof
bool Source_str::get( uchar &ch ) {
  if constexpr ( enable_statis_max_source_get ) {
    ++statis_get;
  }
  bool ok = pos.index < length;
  if ( ok ) {
    ch = str[pos.index];
    ++pos.index;
    ++pos.col;
    if ( ch == '\n' ) {
      ++pos.linenum;
      pos.col = 0;
    }
  } else {
    ch = '\0';
  }
  return ok;
}

bool Source_str::peek( uchar &ch ) {
  if constexpr ( enable_statis_max_source_get ) {
    ++statis_get;
  }
  bool ok = pos.index < length;
  ch = ( ok ) ? str[pos.index] : '\0';
  return ok;
}

void Source_str::getposition( Position &p ) { p = pos; }

void Source_str::setposition( Position p ) {
  if ( 0 <= p.index && p.index <= length ) {
    pos = p;
  } else {
    if ( p.index < 0 ) {
      pos.index = 0;
      pos.linenum = 1;
      pos.col = 0;
    } else {
      if ( p.index > length ) {
        pos.index = length;
      }
    }
  }
}

int Source_str::compare( const Position &bi, const uchar *pch, size_t maxcount ) {
  auto p = str.data() + bi.index;
  return strncmp( p, (cch *)pch, maxcount );
}

bool Source_str::eof() { return ( pos.index >= length ); }

//----------------------------------------------------------------------------------------------
Source_file::Source_file( cch *filename ) : fn( filename ) {
  assert( filename );
  ifstream fs;
  fs.open( fn, ios::in | ios::binary );
  if ( fs.is_open() ) {
    fs.seekg( 0, ios::end );
    length = fs.tellg();
    fs.seekg( 0, ios::beg );
    str.resize( length );
    fs.read( str.data(), length );
  }
  fs.close();
}

// return false if eof
bool Source_file::get( uchar &ch ) {
  if constexpr ( enable_statis_max_source_get ) {
    ++statis_get;
  }

  bool ok = pos.index < length;
  if ( ok ) {
    ch = str[pos.index];
    ++pos.index;
    ++pos.col;
    if ( ch == '\n' ) {
      ++pos.linenum;
      pos.col = 0;
    }
  } else {
    ch = '\0';
  }
  return ok;
}

bool Source_file::peek( uchar &ch ) {
  if constexpr ( enable_statis_max_source_get ) {
    ++statis_get;
  }
  bool ok = pos.index < length;
  ch = ( ok ) ? str[pos.index] : '\0';
  return ok;
}

void Source_file::getposition( Position &p ) { p = pos; }

void Source_file::setposition( Position p ) {
  if ( 0 <= p.index && p.index <= length ) {
    pos = p;
  } else {
    if ( p.index < 0 ) {
      pos.index = 0;
      pos.linenum = 1;
      pos.col = 0;
    } else {
      if ( p.index > length ) {
        pos.index = length;
      }
    }
  }
}

int Source_file::compare( const Position &bi, const uchar *pch, size_t maxcount ) {
  auto p = str.data() + bi.index;
  return strncmp( p, (cch *)pch, maxcount );
}

bool Source_file::eof() { return ( pos.index >= length ); }

//----------------------------------------------------------------------------------------------
Source_token::Source_token( tokens_t &tks ) : data( tks ) { length = data.size(); }

// return false if eof
bool Source_token::get( Token &tk ) {
  if constexpr ( enable_statis_max_source_get ) {
    ++statis_get;
  }

  bool ok = currpos.index < length;
  if ( ok ) {
    tk = data[currpos.index++];
  } else {
    tk.bi = Position();
    tk.ei = Position();
  }
  return ok;
}

bool Source_token::peek( Token &tk ) {
  if constexpr ( enable_statis_max_source_get ) {
    ++statis_get;
  }
  bool ok = currpos.index < length;
  tk = ( ok ) ? data[currpos.index] : Token{};
  return ok;
}

void Source_token::getposition( Position &pos ) { pos.index = currpos.index; }

void Source_token::setposition( Position pos ) {
  auto idx = pos.index;
  if ( 0 <= idx && idx <= length ) {
    currpos.index = idx;
  } else {
    if ( idx < 0 ) {
      currpos.index = 0;
    } else {
      if ( idx > length ) {
        currpos.index = length;
      }
    }
  }
}

int Source_token::compare( const Position &bi, const Token *p, size_t maxcount ) {
  assert( bi.index + maxcount <= length );
  auto count = 0;
  for ( auto i = bi.index; count < maxcount; ++count ) {
    if ( data[i++].nid != p->nid )
      break;
  }
  return ( count == maxcount ) ? 0 : 1;
}

bool Source_token::eof() { return ( currpos.index >= length ); }

//----------------------------------------------------------------------------------------------
Result::Result() {}

intptr_t Result::push_item( const item_t &item ) {
  data.push_back( item );
  return data.size() - 1;
}

void Result::unwind( intptr_t index ) {
  if ( 0 <= index && index < data.size() ) {
    data.erase( data.begin() + index, data.end() );
  } else {
    dbg( index, data.size() );
    assert( false );
  }
}

void Result::print() const {
  printf( "--tokentype, node_id, bi_index, bi_linenum, bi_col, ei_index, ei.linenum, ei.col\n" );
  for ( const auto &i : data ) {
    std::string padding( i.hierarchy * 2, ' ' );
    auto indent = padding.c_str();
    auto compound = COMPOUND( i.classkind );
    switch ( i.type ) {
      case result_type::scope_begin: printf( "%s%s", indent, compound ? "{\n" : "" ); break;
      case result_type::match_content:
        if ( !compound ) {
          printf( "%s{%d,%d,%lld,%lld,%lld,%lld,%lld,%lld}, --%s", "", i.tokentype, i.nid,
                  i.bi.index, i.bi.linenum, i.bi.col, i.ei.index, i.ei.linenum, i.ei.col, i.name );
        } else {
        }
        break;
      case result_type::scope_end:
        if ( compound ) {
          printf( "%s}, --%s\n", indent, i.name );
        } else {
          printf( "\n" );
        }
        break;
    };
  }
}

//----------------------------------------------------------------------------------------------

Nliteral::Nliteral( cch *t, cch *s, int id, int min, int max )
    : Nfunc<uchar>( t, Nliteral::match_func, id, min, max ), literal_str( s ) {
  assert( literal_str );
  str_len = strlen( literal_str );
  match_bi = match_ei = Position{ 0, 1 };
  assert( str_len > 0 );
  assert( first_list );
  first_list->insert( literal_str[0] );
}

roadmap Nliteral::match_func( Nfunc<uchar> &node ) {
  auto ret = roadmap::rangematched;
  auto i = 0;
  uchar ch = '\0';
  auto pnode = dynamic_cast<Nliteral *>( &node );
  assert( pnode );
  auto pSource = node.pSource;
  for ( bool eof = false; i < pnode->str_len; ++i ) {
    eof = !pSource->get( ch );
    if ( eof || *( pnode->literal_str + i ) != ch )
      break;
  }

  ret = ( i == pnode->str_len )
            ? ( pSource->getposition( pnode->match_ei ), roadmap::rangematched )
            : ( pSource->setposition( pnode->match_bi ), roadmap::rangeunmatched );
  return ret;
}

roadmap Nliteral::match() {
  auto ret{ roadmap::rangematched };
  pSource->getposition( match_bi );
  push_range_status( { match_bi } );
  push_result_scope_begin();

  ret = Nliteral::match_func( self );

  match_result_last = ret;

  if ( match_result_last == roadmap::rangematched ) {
    auto result_index = push_result_range_result();
    range_state->push_state( { match_bi, match_ei, result_index } );
  }

  return ret;
}

intptr_t Nliteral::push_result_range_result() {
  intptr_t index = 0;
  if ( match_result_last == roadmap::rangematched ) {
    auto const &p_index = ( has_parent() ) ? parent_index : 0;
    auto &parent_n = ( *pStack->node_list[p_index] );
    Result::item_t rslt;
    rslt.type = result_type::match_content;
    rslt.hierarchy = hierarchy;
    rslt.parent_parse_sn = parent_n.parse_sn;
    rslt.parent_node_id = parent_n.node_id;
    rslt.parse_sn = parse_sn;
    rslt.nid = node_id;
    rslt.classkind = classkind;
    rslt.tokentype = tokentype;
    rslt.name = name;
    rslt.bi = match_bi;
    rslt.ei = match_ei;
    index = pResult->push_item( rslt );
  }
  return index;
}

Node_ptr<uchar> Nliteral::clone() {
  static Pool<Nliteral> pool;
  return { pool.clone( self ), bnf::Node_deleter<uchar> };
}

void Nliteral::release() {
  assert( pool_object );
  Node::release();
  reinterpret_cast<Pool_t *>( pool_object )->release( this );
}

//----------------------------------------------------------------------------------------------
roadmap bnf::SPACE_F( Nfunc<uchar> &node ) {
  auto rdm = roadmap::unmatched;

  auto matched_ch_count = 0;
  uchar ch = '\0';

  assert( node.pSource );
  auto &src = *node.pSource;
  auto temp = node.match_bi;
  while ( src.get( ch ) && ( ' ' == ch || '\n' == ch || '\t' == ch || '\r' == ch ) ) {
    ++matched_ch_count;
    node.pSource->getposition( temp );
  };

  if ( matched_ch_count > 0 ) {
    node.match_ei = temp;
    node.pSource->setposition( temp );
    rdm = roadmap::matched;
  } else {
    node.pSource->setposition( node.match_bi );
    rdm = roadmap::unmatched;
  }

  return rdm;
}

//----------------------------------------------------------------------------------------------
// SPACE_F RoadSign
roadmap bnf::SPACE_FRS( const Symbol &symbol ) {
  auto rdm = roadmap::unmatched;
  auto &ch = symbol.ch;
  if ( ' ' == ch || '\n' == ch || '\t' == ch || '\r' == ch )
    rdm = roadmap::matched;
  return rdm;
}
//----------------------------------------------------------------------------------------------
roadmap bnf::ID_F( Nfunc<uchar> &node ) {
  auto rdm = roadmap::unmatched;

  // 状态
  enum class S : i32 {
    S0 = 0, // 开始状态
    CTN,    // 内容部分
    LAST,   // 标志
    ERR = 0x10,
    SUC,
    EBD = 0x100,
  } state{ S::S0 };

  // 输入
  enum class I : i32 {
    underscore, // _
    alpha,      // [a-zA-Z]
    digit,      // [0-9]
    mb,         // ch & 0x8F > 0
    rest,       // 除引号之外
    LAST,       // 标志
  } input{ I::rest };

  //  _,[a-ZA-Z],[0-9],mb,rest
  S N[(i32)S::LAST][(i32)I::LAST] = {
      { S::CTN, S::CTN, S::ERR, S::CTN, S::ERR }, // S0
      { S::CTN, S::CTN, S::CTN, S::CTN, S::EBD }, // CTN
  };

  auto source = node.pSource;
  auto mapinput = []( uchar ch ) -> I {
    // regard eof as rest
    auto ret = I::rest;
    if ( ch == '_' )
      ret = I::underscore;
    else if ( ch >= '0' && ch <= '9' )
      ret = I::digit;
    else if ( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) )
      ret = I::alpha;
    else if ( ch & 0x80 )
      ret = I::mb;
    return ret;
  };

  auto ok = true;
  do {
    uchar ch = '\0';
    ok = source->get( ch );
    input = mapinput( ch );
    state = N[(i32)state][(i32)input];
    ////dbg( ch, input, state, node.match_bi.index );
    if ( ( state != S::ERR ) && ( state != S::SUC ) && ( state != S::EBD ) ) {
      source->getposition( node.match_ei );
    } else {
      break;
    }
  } while ( ok );

  rdm = ( state == S::SUC || state == S::EBD ) ? roadmap::matched : roadmap::unmatched;
  if ( roadmap::matched == rdm ) {
    if ( state == S::EBD ) {
      source->setposition( node.match_ei ); // 将位置移回匹配范围(放回多读取的边界字符)
    }
  } else {
    source->setposition( node.match_bi );
  }

  return rdm;

} // ID_F

//----------------------------------------------------------------------------------------------
// ID_F RoadSign
roadmap bnf::ID_FRS( const Symbol &symbol ) {
  auto rdm = roadmap::unmatched;
  auto &ch = symbol.ch;
  if ( ( ch == '_' ) || ( ch & 0x80 ) ||
       ( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) ) )
    rdm = roadmap::matched;
  return rdm;
}

//----------------------------------------------------------------------------------------------
// string ==> "...\"..."
roadmap bnf::SLASHSTRING_F( Nfunc<uchar> &node ) {
  auto rdm = roadmap::unmatched;

  uchar ch = '\0';

  auto ok = node.pSource->get( ch );
  if ( ch == '"' ) {
    auto index_slash = 0;
    auto matched_ch_count = 1;
    do {
      ok = node.pSource->get( ch );
      ++matched_ch_count;
      if ( ch == '\\' ) {
        index_slash = matched_ch_count;
      }
    } while ( ( ( index_slash - matched_ch_count == 1 ) || ( '"' != ch ) ) && ( '\0' != ch ) );

    if ( ( matched_ch_count >= 2 ) && ( '"' == ch ) ) {
      node.pSource->getposition( node.match_ei );
      rdm = roadmap::matched;
    }
  }

  if ( roadmap::unmatched == rdm ) {
    node.pSource->setposition( node.match_bi );
  }
  return rdm;
}

roadmap bnf::SLASHSTRING_FRS( const Symbol &symbol ) {
  return ( symbol.ch == '"' ) ? roadmap::matched : roadmap::unmatched;
}

//----------------------------------------------------------------------------------------------
// string ==> '...\'...'
roadmap bnf::QUOTESTRING_F( Nfunc<uchar> &node ) {
  auto rdm = roadmap::unmatched;

  // 状态
  enum class S : i32 {
    S0 = 0, // 开始状态
    CTN,    // 内容部分
    SLA,    // 反斜线部分
    LAST,   // 标志
    ERR,
    SUC,
  } state{ S::S0 };

  // 输入
  enum class I : i32 {
    quote, // 引号
    slash, // 反斜线
    rest,  // 除引号之外
    LAST,  // 标志
  } input{ I::rest };

  //  quote,slash,rest,
  S N[(i32)S::LAST][(i32)I::LAST] = {
      { S::CTN, S::ERR, S::ERR }, // S0
      { S::SUC, S::SLA, S::CTN }, // CTN
      { S::CTN, S::CTN, S::CTN }, // SLA
  };

  auto mapinput = []( uchar ch ) -> I {
    // regard eof as rest
    auto ret = I::rest;
    if ( ch == '\'' )
      ret = I::quote;
    else if ( ch == '\\' )
      ret = I::slash;
    return ret;
  };

  auto ok = true;
  do {
    uchar ch = '\0';
    ok = node.pSource->get( ch );
    input = mapinput( ch );
    state = N[(i32)state][(i32)input];
    if ( ( state != S::ERR ) && ( state != S::SUC ) ) {
      node.pSource->getposition( node.match_ei );
    } else {
      break;
    }
  } while ( ok );

  if ( state == S::SUC ) {
    rdm = roadmap::matched;
  } else {
    rdm = roadmap::unmatched;
    node.pSource->setposition( node.match_bi );
  }

  return rdm;
}

roadmap bnf::QUOTESTRING_FRS( const Symbol &symbol ) {
  return ( symbol.ch == '\'' ) ? roadmap::matched : roadmap::unmatched;
}

//----------------------------------------------------------------------------------------------
// string ==> "...\"..."
roadmap bnf::DQUOTESTRING_F( Nfunc<uchar> &node ) {
  auto rdm = roadmap::unmatched;

  // 状态
  enum class S : i32 {
    S0 = 0, // 开始状态
    CTN,    // 内容部分
    SLA,    // 反斜线部分
    LAST,   // 状态数量标志
    ERR,
    SUC,
  } state{ S::S0 };

  // 输入
  enum class I : i32 {
    quote, // 引号
    slash, // 反斜线
    rest,  // 除引号之外
    LAST,  // 标志
  } input{ I::rest };

  // quote,slash,rest,
  S N[(i32)S::LAST][(i32)I::LAST] = {
      { S::CTN, S::ERR, S::ERR }, // S0
      { S::SUC, S::SLA, S::CTN }, // CTN
      { S::CTN, S::CTN, S::CTN }, // SLA
  };

  auto mapinput = []( uchar ch ) -> I {
    // regard eof as rest
    auto ret = I::rest;
    if ( ch == '"' )
      ret = I::quote;
    else if ( ch == '\\' )
      ret = I::slash;
    return ret;
  };

  auto ok = true;
  do {
    uchar ch = '\0';
    ok = node.pSource->get( ch );
    input = mapinput( ch );
    state = N[(i32)state][(i32)input];
    if ( ( state != S::ERR ) && ( state != S::SUC ) ) {
      node.pSource->getposition( node.match_ei );
    } else {
      break;
    }
  } while ( ok );

  if ( state == S::SUC ) {
    rdm = roadmap::matched;
  } else {
    rdm = roadmap::unmatched;
    node.pSource->setposition( node.match_bi );
  }

  return rdm;
}

roadmap bnf::DQUOTESTRING_FRS( const Symbol &symbol ) {
  return ( symbol.ch == '"' ) ? roadmap::matched : roadmap::unmatched;
}

//----------------------------------------------------------------------------------------------
// number ==> [+-]/digit*[./digit*][eE[+-]/digit+]
roadmap bnf::REALNUMBER_F( Nfunc<uchar> &node ) {
  auto rdm = roadmap::unmatched;

  enum class S : i32 {
    S0 = 0, // 开始状态
    NUM,    // 数字部分
    INT,    // 整数状态
    DT0,    // 没有数字就遇到dot状态
    DT1,    // 有数字才遇到dot状态
    FRA,    // 小数状态
    EXP,    // 幂
    PW1,    // 幂部分已经有了数字
    POW,    // 开始接收幂的数字
    LAST,
    ERR,
    SUC,
    EBD, // 当前输入不属于匹配范围，已匹配的可以构成一个整体
  } state{ S::S0 };

  enum class I : i32 {
    ss = 0, // 数字正负符号
    digit,  // （0-9）数字
    dot,    // 点号
    exp,    // （e或E）幂标志
    rest,   // 除以上面的符号
    LAST,
  } input{ I::ss };

  // 边界符号不应包括进匹配范围
  // 该匹配没有明确边界符号，需要外部符号来帮助判断是否匹配终止，实际上SUC==EBD
  // ss,digit,dot,exp,rest
  S N[(i32)S::LAST][(i32)I::LAST] = {
      { S::NUM, S::INT, S::DT0, S::ERR, S::ERR }, // S0
      { S::ERR, S::INT, S::DT0, S::ERR, S::ERR }, // NUM
      { S::EBD, S::INT, S::DT1, S::EXP, S::EBD }, // INT
      { S::ERR, S::FRA, S::ERR, S::ERR, S::ERR }, // DT0
      { S::EBD, S::FRA, S::EBD, S::EXP, S::EBD }, // DT1
      { S::EBD, S::FRA, S::EBD, S::EXP, S::EBD }, // FRA
      { S::POW, S::PW1, S::ERR, S::ERR, S::ERR }, // EXP
      { S::EBD, S::PW1, S::EBD, S::EBD, S::EBD }, // PW1
      { S::ERR, S::PW1, S::ERR, S::ERR, S::ERR }, // POW
  };

  auto mapinput = []( uchar ch ) -> I {
    // regard eof as rest
    auto ret = I::rest;
    if ( ch == '+' || ch == '-' )
      ret = I::ss;
    else if ( ch >= '0' && ch <= '9' )
      ret = I::digit;
    else if ( ch == '.' )
      ret = I::dot;
    else if ( ch == 'e' || ch == 'E' )
      ret = I::exp;
    return ret;
  };

  auto ok = true;
  do {
    uchar ch = '\0'; // 当eof时，输入的值
    ok = node.pSource->get( ch );
    input = mapinput( ch );
    state = N[(i32)state][(i32)input];
    // dbg( ch, input, state );
    if ( ( state != S::ERR ) && ( state != S::SUC ) && ( state != S::EBD ) ) {
      node.pSource->getposition( node.match_ei );
    } else {
      break;
    }
  } while ( ok );

  rdm = ( state == S::SUC || state == S::EBD ) ? roadmap::matched : roadmap::unmatched;
  if ( roadmap::matched == rdm ) {
    if ( state == S::EBD ) {
      node.pSource->setposition( node.match_ei ); // 将位置移回匹配范围(放回多读取的边界字符)
    }
  } else {
    node.pSource->setposition( node.match_bi );
  }

  return rdm;
}

roadmap bnf::REALNUMBER_FRS( const Symbol &symbol ) {
  auto rdm = roadmap::unmatched;
  auto &ch = symbol.ch;
  if ( '+' == ch || '-' == ch || '.' == ch || ( '0' <= ch && '9' >= ch ) )
    rdm = roadmap::matched;
  return rdm;
}
//----------------------------------------------------------------------------------------------
// number ==> digit*[./digit*][eE[+-]/digit+]
roadmap bnf::REALNUMBER_NS_F( Nfunc<uchar> &node ) {
  auto rdm = roadmap::unmatched;

  enum class S : i32 {
    S0 = 0, // 开始状态
    NUM,    // 数字部分
    INT,    // 整数状态
    DT0,    // 没有数字就遇到dot状态
    DT1,    // 有数字才遇到dot状态
    FRA,    // 小数状态
    EXP,    // 幂
    PW1,    // 幂部分已经有了数字
    POW,    // 开始接收幂的数字
    LAST,
    ERR,
    SUC,
    EBD, // 当前输入不属于匹配范围，已匹配的可以构成一个整体
  } state{ S::S0 };

  enum class I : i32 {
    digit, // （0-9）数字
    dot,   // 点号
    exp,   // （e或E）幂标志
    rest,  // 除以上面的符号
    LAST,
  } input{ I::rest };

  // 边界符号不应包括进匹配范围
  // 该匹配没有明确边界符号，需要外部符号来帮助判断是否匹配终止，实际上SUC==EBD
  // ss,digit,dot,exp,rest
  S N[(i32)S::LAST][(i32)I::LAST] = {
      { S::INT, S::DT0, S::ERR, S::ERR }, // S0
      { S::INT, S::DT0, S::ERR, S::ERR }, // NUM
      { S::INT, S::DT1, S::EXP, S::EBD }, // INT
      { S::FRA, S::ERR, S::ERR, S::ERR }, // DT0
      { S::FRA, S::EBD, S::EXP, S::EBD }, // DT1
      { S::FRA, S::EBD, S::EXP, S::EBD }, // FRA
      { S::PW1, S::ERR, S::ERR, S::ERR }, // EXP
      { S::PW1, S::EBD, S::EBD, S::EBD }, // PW1
      { S::PW1, S::ERR, S::ERR, S::ERR }, // POW
  };

  auto mapinput = []( uchar ch ) -> I {
    // regard eof as rest
    auto ret = I::rest;
    if ( ch >= '0' && ch <= '9' )
      ret = I::digit;
    else if ( ch == '.' )
      ret = I::dot;
    else if ( ch == 'e' || ch == 'E' )
      ret = I::exp;
    return ret;
  };

  auto ok = true;
  do {
    uchar ch = '\0'; // 当eof时，输入的值
    ok = node.pSource->get( ch );
    input = mapinput( ch );
    state = N[(i32)state][(i32)input];
    ////dbg( ch, input, state );
    if ( ( state != S::ERR ) && ( state != S::SUC ) && ( state != S::EBD ) ) {
      node.pSource->getposition( node.match_ei );
    } else {
      break;
    }
  } while ( ok );

  rdm = ( state == S::SUC || state == S::EBD ) ? roadmap::matched : roadmap::unmatched;
  if ( roadmap::matched == rdm ) {
    if ( state == S::EBD ) {
      node.pSource->setposition( node.match_ei ); // 将位置移回匹配范围(放回多读取的边界字符)
    }
  } else {
    node.pSource->setposition( node.match_bi );
  }
  return rdm;
}

roadmap bnf::REALNUMBER_NS_FRS( const Symbol &symbol ) {
  auto rdm = roadmap::unmatched;
  auto &ch = symbol.ch;
  if ( '.' == ch || ( '0' <= ch && ch <= '9' ) )
    rdm = roadmap::matched;
  return rdm;
}
