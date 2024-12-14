#include "bnf.h"
#include "llua.h"
using namespace bnf;
using namespace llua;

//----------------------------------------------------------------------------------------------
// extern Pool<Range::item_t> rangepool;

//----------------------------------------------------------------------------------------------
void test_bypass() {
  bnf::Source_str source( "aaaab" );
  bnf::Result result;
  bnf::Stack<uchar> stack;

  enum NODE_ID {
    nid_S,
    nid_a,
    nid_b,
  };
  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nliteral a{ "a", "a", nid_a, 1, 4 };
    bnf::Nliteral b{ "b", "b", nid_b };

    bnf::Nseque<uchar> S{ "S" };
    S( a, b );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto ret = parser.parse( G );
  printf( "--test_bypass" );
  print_result( parser );
}

//----------------------------------------------------------------------------------------------
void test_alter() {
  bnf::Source_str source( "aaab" );
  bnf::Result result;
  bnf::Stack<uchar> stack;

  // G=>S
  // S=>A
  // A=>("a" | "b"){0,4}
  enum NODE_ID {
    nid_S, // 0
    nid_A,
    nid_a,
    nid_b,
  };
  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nliteral a{ "a", "a", nid_a };
    bnf::Nliteral b{ "b", "b", nid_b };
    bnf::Nalter<uchar> A{ "A", nid_A, 0, 4 };
    A( a, b );

    bnf::Nseque<uchar> S{ "S" };
    A.r_conf.hierarchy = false;
    S( A );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto ret = parser.parse( G );
  printf( "--test_alter" );
  print_result( parser );
}

//----------------------------------------------------------------------------------------------
void test_seque() {
  bnf::Source_str source( "abcabc" );
  bnf::Result result;
  bnf::Stack<uchar> stack;

  enum NODE_ID {
    nid_S = 127,
    nid_A,
    nid_a,
    nid_b,
    nid_c,
  };
  // G=>S
  // S=>A{0,2}
  // A=>"abc"
  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nliteral a{ "a", "a", nid_a };
    bnf::Nliteral b{ "b", "b", nid_b };
    bnf::Nliteral c{ "c", "c", nid_c };
    bnf::Nseque<uchar> A{ "A", nid_A, 0, 2 };
    A( a, b, c );

    bnf::Nseque<uchar> S{ "S" };
    S( A );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto ret = parser.parse( G );
  printf( "--test_seque" );
  print_result( parser );
}

//----------------------------------------------------------------------------------------------
void test_unwind() {
  bnf::Source_str source( "abcabc" );
  bnf::Result result;
  bnf::Stack<uchar> stack;

  enum NODE_ID {
    nid_S = 127,
    nid_A,
    nid_B,
    nid_C,
    nid_a,
    nid_b,
    nid_c,
  };
  // G=>S
  // S=>C
  // C=>A|B
  // A=>"abc"{3,3}
  // B=>"abc"{0,2}
  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nliteral a{ "a", "a", nid_a };
    bnf::Nliteral b{ "b", "b", nid_b };
    bnf::Nliteral c{ "c", "c", nid_c };
    bnf::Nseque<uchar> A{ "A", nid_A, 3, 3 };
    A( a, b, c );
    bnf::Nseque<uchar> B{ "B", nid_B, 0, 2 };
    B( a, b, c );

    bnf::Nalter<uchar> C{ "C", nid_C };
    C( A, B );
    bnf::Nseque<uchar> S{ "S", nid_S };
    C.r_conf.hierarchy = false;
    S( C );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto ret = parser.parse( G );
  printf( "--test_unwind" );
  print_result( parser );
}

//----------------------------------------------------------------------------------------------
void test_realnumber() {
  bnf::Source_str source( R"--(-3.14159e-2)--" );

  bnf::Result result;
  bnf::Stack<uchar> stack;

  enum NODE_ID {
    nid_S = 127,
    nid_number,
  };
  enum TT {
    ttRealnumber,
  };
  // G=>S
  // S=>REALNUMBER
  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nfunc<uchar> REALNUMBER{ "REALNUMBER", bnf::REALNUMBER_F, bnf::REALNUMBER_FRS,
                                  nid_number };
    bnf::Nseque<uchar> S{ "S", nid_S };
    REALNUMBER.tokentype = TT::ttRealnumber;
    S( REALNUMBER );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto ret = parser.parse( G );
  printf( "--test_realnumber" );
  print_result( parser );
}

//----------------------------------------------------------------------------------------------
void test_llua_string() {
  // bnf::Source_str source( R"--([[abc]])--");
  bnf::Source_str source( R"--([==[abc]= ]==])--" );

  bnf::Result result;
  bnf::Stack<uchar> stack;

  enum NODE_ID {
    nidS = 127,
    nidLuaString,
  };
  enum TT {
    ttLuaString = 1,
  };
  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nfunc<uchar> LUASTRING{ "LUASTRING", llua::String_F, llua::String_FRS, nidLuaString };
    bnf::Nseque<uchar> S{ "S", nidS };
    LUASTRING.tokentype = TT::ttLuaString;
    S( LUASTRING );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto ret = parser.parse( G );
  print_result( parser );
}
/*
 */

//----------------------------------------------------------------------------------------------
/*
void test_llua_comment() {
  // bnf::Source_str source( R"--(--[[abc]])--");
  // bnf::Source_str source( R"--(--abc)--");
  // bnf::Source_str source( R"--(
  //  --abc
  //  --xxx
  //  )--");
  // bnf::Source_str source( R"--(-- [==[ abc]= ]==])--" );
  bnf::Source_str source( R"+(
    as --c1)(
    ad --d2
    )+" );

  bnf::Result result;
  bnf::Stack<uchar> stack;

  enum NODE_ID {
    nidS = 0,
    nidLuaComment,
    nidSpace,
    nidId,
  };

  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nfunc<uchar> LUACOMMENT{ "comment", llua::Comment_F, nidLuaComment, 1, 1, nullptr };
    bnf::Nfunc<uchar> SPACE{ "space", SPACE_F, nidSpace };
    bnf::Nfunc<uchar> ID{ "id", ID_F, nidId };
    bnf::Nalter<uchar> S{ "S", nidS, 0, -1 };
    S( LUACOMMENT, SPACE, ID );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto rdm = parser.parse( G );
  if ( rdm == roadmap::fail ) {
    parser.last_match.print( result );
    parser.last_unmatch.print( result );
  }
  print_result(parser);
}
*/

//----------------------------------------------------------------------------------------------
/*
void test_llua_operator() {
  // bnf::Source_str source( R"--([[abc]])--");
  bnf::Source_str source( R"--(>= :: ... //)--" );

  bnf::Result result;
  bnf::Stack<uchar> stack;

  enum NODE_ID {
    nidS = 127,
    nidLuaOperator,
    nidSpace,
  };
  enum TT {
    ttLuaOperator = 0,
  };

  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nalter<uchar> S{ "S", nidS, 0, -1 };
    bnf::Nfunc<uchar> LUAOPERATOR{ "LUAOPERATOR", llua::OPERATOR_F, nidLuaOperator, 1, 1, nullptr };
    LUAOPERATOR.tokentype = TT::ttLuaOperator;
    bnf::Nfunc<uchar> SPACE{ "SPACE", bnf::SPACE_F, nidSpace, 1, 1, nullptr };
    SPACE.r_conf.visible = true;

    S.pool_candidate_flag = false;
    S( SPACE, LUAOPERATOR );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto ret = parser.parse( G );
  print_result(parser);
}
*/

//----------------------------------------------------------------------------------------------
void test_strings() {
  bnf::Source_str source(
      R"--("this is short string" 
    "width slash (\t)" -3.14159e-2)--" );

  bnf::Result result;
  bnf::Stack<uchar> stack;

  enum NODE_ID {
    nid_S = 127,
    nid_string,
    nid_space,
    nid_number,
  };
  enum TT {
    ttString = 1,
    ttSpace,
    ttRealnumber,
  };
  // G=>S
  // S=>String Space String
  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nfunc<uchar> STRING{ "STRING", bnf::SLASHSTRING_F, bnf::SLASHSTRING_FRS, nid_string };
    bnf::Nfunc<uchar> SPACE{ "SPACE", bnf::SPACE_F, bnf::SPACE_FRS, nid_space };
    bnf::Nfunc<uchar> REALNUMBER{ "REALNUMBER", bnf::REALNUMBER_F, bnf::REALNUMBER_FRS,
                                  nid_number };
    bnf::Nseque<uchar> S{ "S", nid_S };
    STRING.tokentype = TT::ttString;
    SPACE.tokentype = TT::ttSpace;
    REALNUMBER.tokentype = TT::ttRealnumber;

    S( STRING, SPACE, STRING, SPACE, REALNUMBER );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto ret = parser.parse( G );
  printf( "--test_strings" );
  print_result( parser );
}

//----------------------------------------------------------------------------------------------
void test_range() {
  bnf::Source_str source( "abcabc" );
  bnf::Result result;
  bnf::Stack<uchar> stack;

  enum NODE_ID {
    nid_S,
    nid_abc,
  };
  // G::=S
  // S::="abc"{1,2}
  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nliteral s{ "abc", "abc", nid_abc, 1, 2 };
    bnf::Nseque<uchar> S{ "S" };
    S( s );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto ret = parser.parse( G );
  printf( "--test_range" );
  print_result( parser );
}

//----------------------------------------------------------------------------------------------
void test_ids() {
  // bnf::Source_str source( "John Backus   Peter Naur" );
  bnf::Source_str source( "Smith Äá¹ÅÀ­ Backus Peter" );
  // bnf::Source_str source( "John Backus 123 Peter Naur" );
  // bnf::Source_str source( "John" );
  bnf::Result result;
  bnf::Stack<uchar> stack;

  enum NODE_ID {
    nid_S = 127,
    nid_id,
    nid_id_tail,
    nid_space,
  };
  // G=>S
  // S=>ID { SPACE ID }

  bnf::Node<uchar> *G = nullptr;
  {
    bnf::Nseque<uchar> S{ "S" };
    bnf::Nfunc<uchar> astID{ "astID", bnf::ID_F, bnf::ID_FRS, nid_id };
    bnf::Nfunc<uchar> astSPACE{ "astSPACE", bnf::SPACE_F, bnf::SPACE_FRS, nid_space };
    bnf::Nseque<uchar> astTAIL{ "astTAIL", nid_id_tail, 0, bnf::notset };
    astSPACE.r_conf.visible = false;
    astTAIL.r_conf.hierarchy = false;

    astTAIL( astSPACE, astID );
    S( astID, astTAIL );
    G = bnf::pool_tree( S );
  }
  assert( G );

  bnf::Parser<uchar> parser{ source, stack, result };

  auto ret = parser.parse( G );
  printf( "--test_ids" );
  print_result( parser );
}

//----------------------------------------------------------------------------------------------
template <typename E> void f( E & ) {}
//----------------------------------------------------------------------------------------------
int main( int argc, const char **argv ) {
  printf( "\n---begin---\n" );

  if ( argc >= 2 ) {
    llua::parse_file( argv[1] );
  } else {
    /*
      test_llua_string();
      test_llua_operator();
      test_llua_comment();
      */
    /*
    test_unwind();
    test_range();
    test_ids();
    test_strings();
    test_realnumber();
    test_seque();
    test_bypass();
     */
    //test_alter();
  }

  printf( "\n---end---\n" );

  return 0;
}
