// Backus-Naur Form
#ifndef __bnf_h__
#define __bnf_h__
#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

//----------------------------------------------------------------------------------------------
// 2021-10-28: 加入FIRST和FOLLOW功能
// 允许有共同前缀，有回溯
//----------------------------------------------------------------------------------------------
//-DDBG_MACRO_DISABLE
#include "dbg.h"

//----------------------------------------------------------------------------------------------
namespace bnf {

constexpr auto enable_statis_max_source_get = true;
constexpr auto enable_statis_max_stack_node_count = true;
constexpr auto enable_delay_product = true;
constexpr auto max_static_roadsign_branch = 64;

constexpr auto FIRST_NODE_INDEX = 0;
// using uchar = unsigned char;
using uchar = char;
using cch = const char;
using i32 = std::int32_t;

#define self ( *this )

template <typename E> class Parser;
template <typename E> class Source;
template <typename E> class Stack;
template <typename E> class Node;
template <typename E> class Ncompound;
template <typename E> class Nalter;
template <typename E> class Nseque;
template <typename E> class Nfunc;
template <typename E> struct RSWList;
template <typename E> struct RSWItem;
class Result;
struct RoadSignSet;

constexpr auto notset = -1;
constexpr auto unlimit = -1;

// enum class roadmap : int
enum roadmap {
  init = -2,
  fail = -1,
  success = 0,
  matching = 1,
  producted = 2,
  matched = 4,
  unmatched = 8,
  collcapse = 16,
  bypassmatched = 32, //ε
  rangematched = 64,
  rangeunmatched = 128,
};

enum node_state {
  on_duty = 1,
  //// node_producted = 2,
};

//----------------------------------------------------------------------------------------------
enum result_type {
  scope_begin,
  match_content,
  scope_end,
};

//----------------------------------------------------------------------------------------------
struct Position {
  intptr_t index;
  intptr_t linenum;
  intptr_t col;
};
bool operator<( const Position &l, const Position &r );

//----------------------------------------------------------------------------------------------
using Node_id = std::uint32_t;

//----------------------------------------------------------------------------------------------
struct Token {
  Node_id nid;
  Position bi;
  Position ei;
};
bool operator<( const Token &l, const Token &r );
using tokens_t = std::vector<Token>;

using Lalter = bnf::Nalter<bnf::uchar>;
using Lseque = bnf::Nseque<bnf::uchar>;
using Lfunc = bnf::Nfunc<bnf::uchar>;

using Salter = bnf::Nalter<Token>;
using Sseque = bnf::Nseque<Token>;
using Sfunc = bnf::Nfunc<Token>;

//----------------------------------------------------------------------------------------------
void print_tokens( const tokens_t &tks );

//----------------------------------------------------------------------------------------------
enum ClassKind {
  default_ = 0,
  seque,
  alter,
  func = 0x100,
};
#define COMPOUND( classkind )                                                                      \
  ( ( classkind ) == ClassKind::alter || ( classkind ) == ClassKind::seque )

//----------------------------------------------------------------------------------------------
struct Result_conf {
  bool visible = true;
  bool hierarchy = true;
};

//----------------------------------------------------------------------------------------------
struct Result {
  struct item_t {
    result_type type;
    int32_t tokentype;
    int hierarchy;
    int parent_parse_sn;
    int parent_node_id;
    int parse_sn;
    ClassKind classkind;
    const char *name;
    Node_id nid;
    Position bi;
    Position ei;
  };
  Result();
  std::vector<item_t> data;
  intptr_t push_item( const item_t & );
  size_t size() { return data.size(); }
  intptr_t max_index() { return data.size() - 1; }
  void unwind( intptr_t index );
  void print() const;
};

//----------------------------------------------------------------------------------------------
template <typename Parser> void print_result( const Parser &parser ) {
  assert( parser.pSource && parser.pStack && parser.pResult );
  printf( R"(
bnf={
)" );
  parser.pResult->print();
  printf( R"(
parser={
  parse = { rdm=%d, matched=%lld, unmatch=%lld, loop_count=%lld },
  result = { size=%llu },
  source = { get=%lld },
  stack = { node_count=%llu, max_count=%llu },
  rangepool = { resuse_count=%llu, max_count=%llu },
},
)",
          parser.rdm, parser.compound_matched_count, parser.compound_unmatch_count,
          parser.loop_count, parser.pResult->data.size(), parser.pSource->statis_get,
          parser.pStack->statis_node_count, parser.pStack->statis_max_count,
          parser.pRangepool->statis_reuse_count, parser.pRangepool->statis_max_count );
  printf( R"(
}
)" );
}

//----------------------------------------------------------------------------------------------
template <typename E> class Source {
public:
  virtual bool get( E &ch ) = 0;              // 获取一个字符,并返回其位置
  virtual bool peek( E &ch ) = 0;             // 只看当前的，不改变游标位置
  virtual void getposition( Position & ) = 0; // 返回当前位置
  virtual void setposition( Position ) = 0;   // 设置当前位置
  virtual int compare( const Position &bi, const E *p,
                       size_t maxcount ) = 0; // 从指定位置比较字符串
  virtual bool eof() = 0;                     // EOF

  int64_t statis_get = 0;
}; // Source

//----------------------------------------------------------------------------------------------
class Source_str : public Source<uchar> {
public:
  Source_str( cch *pch, intptr_t len = -1 );
  virtual bool get( uchar &ch ) override;
  virtual bool peek( uchar &ch ) override;
  virtual void getposition( Position & ) override;
  virtual void setposition( Position ) override;
  virtual int compare( const Position &bi, const uchar *pch, size_t maxcount ) override;
  virtual bool eof() override;

protected:
  std::string str;
  Position pos{ 0, 1, 0 };
  intptr_t length{ 0 };
}; // Source_str

//----------------------------------------------------------------------------------------------
class Source_file : public Source<uchar> {
public:
  Source_file( cch *fn );
  virtual bool get( uchar &ch ) override;
  virtual bool peek( uchar &ch ) override;
  virtual void getposition( Position & ) override;
  virtual void setposition( Position ) override;
  virtual int compare( const Position &bi, const uchar *pch, size_t maxcount ) override;
  virtual bool eof() override;

protected:
  std::string fn;
  std::string str;
  Position pos{ 0, 1, 0 };
  intptr_t length{ 0 };
}; // Source_file

//----------------------------------------------------------------------------------------------
class Source_token : public Source<Token> {
public:
  Source_token( tokens_t &tks );
  virtual bool get( Token &tk ) override;
  virtual bool peek( Token &tk ) override;
  virtual void getposition( Position & ) override;
  virtual void setposition( Position ) override;
  virtual int compare( const Position &bi, const Token *tk, size_t maxcount ) override;
  virtual bool eof() override;

protected:
  tokens_t &data;
  Position currpos{ 0, 0, 0 };
  intptr_t length = 0;
}; // Source_token

//----------------------------------------------------------------------------------------------
struct Pool_item_t {
  intptr_t pool_index = bnf::notset; // 在pool中的索引,可由于判断对象是否在pool中
  void *pool_object = nullptr;       // Pool
};

struct Pool_base_t {
  intptr_t statis_reuse_count = 0;
  intptr_t statis_max_count = 0;
};

template <typename T> struct Pool : Pool_base_t {
  using value_t = T;
  virtual ~Pool() { clear(); }

  struct item_t {
    int prev_index = bnf::notset;
    int next_index = bnf::notset;
    T *pobject = nullptr;
  };
  std::vector<item_t> list;
  int free_head = bnf::notset;
  int busy_head = bnf::notset;
  intptr_t max_pool = 0;
  std::allocator<T> pool_alloc;

  T *clone( const T &r );
  T *alloc();
  void clear() {
    for ( auto &elem : list ) {
      auto &pobj = elem.pobject;
      elem.pobject->~T();               // XXX: 只析构,不释放空间
      pool_alloc.deallocate( pobj, 1 ); // XXX: 归还空间
    }
    list.clear();
  }
  void release( T *storage );
  void travel( const std::function<void( int, int, T * )> &cf );
}; // Pool

template <typename T> void Pool<T>::travel( const std::function<void( int, int, T * )> &cf ) {
  auto index = busy_head;
  auto count = 0;
  while ( index != bnf::notset ) {
    assert( index >= 0 && index < max_pool );
    auto item = list[index];
    cf( ++count, index, item.pobject );
    index = item.next_index;
  }
}

template <typename T> void pool_travel( const T &node ) {
  auto pool = reinterpret_cast<Pool<T> *>( node.pool_object );
  if ( pool ) {
    auto cf = []( int count, int index, T *pnode ) -> void {
      dbg( count, index, pnode->name, pnode );
    };
    pool->travel( cf );
  }
}

template <typename T> T *Pool<T>::alloc() {
  T *storage = nullptr;
  if ( free_head == bnf::notset ) {
    storage = pool_alloc.allocate( 1 );
    assert( nullptr != storage );
    new ( storage ) T(); // XXX: default constructor
    int index = list.size();
    storage->pool_index = index;
    storage->pool_object = this;
    item_t elem;
    elem.prev_index = bnf::notset;
    elem.next_index = busy_head;
    elem.pobject = storage;
    list.push_back( elem );
    ++max_pool;
    if ( busy_head > bnf::notset ) {
      list[busy_head].prev_index = index;
    }
    busy_head = index;
    statis_max_count = max_pool;
  } else {
    auto index = free_head;
    auto &elem = list[index];
    free_head = elem.next_index;
    if ( free_head > bnf::notset ) {
      list[free_head].prev_index = bnf::notset;
    }
    elem.next_index = busy_head;
    if ( busy_head > bnf::notset ) {
      list[busy_head].prev_index = index;
    }
    elem.prev_index = bnf::notset;
    busy_head = index;
    storage = elem.pobject;
    storage->pool_index = index;
    storage->pool_object = this;
    ++statis_reuse_count;
  }
  return storage;
} // alloc

template <typename T> T *Pool<T>::clone( const T &r ) {
  T *storage = nullptr;
  if ( free_head == bnf::notset ) {
    storage = pool_alloc.allocate( 1 );
    assert( nullptr != storage );
    new ( storage ) T( r ); // XXX: placement new
    int index = list.size();
    storage->pool_index = index;
    storage->pool_object = this;
    item_t elem;
    elem.prev_index = bnf::notset;
    elem.next_index = busy_head;
    elem.pobject = storage;
    list.push_back( elem );
    ++max_pool;
    if ( busy_head > bnf::notset ) {
      list[busy_head].prev_index = index;
    }
    busy_head = index;
    statis_max_count = max_pool;
  } else {
    auto index = free_head;
    auto &elem = list[index];
    free_head = elem.next_index;
    if ( free_head > bnf::notset ) {
      list[free_head].prev_index = bnf::notset;
    }
    elem.next_index = busy_head;
    if ( busy_head > bnf::notset ) {
      list[busy_head].prev_index = index;
    }
    elem.prev_index = bnf::notset;
    busy_head = index;
    storage = elem.pobject;
    ( *storage ) = r; // XXX: using operator= instead desctruct and construct
    storage->pool_index = index;
    storage->pool_object = this;
    ++statis_reuse_count;
  }
  return storage;
} // clone

template <typename T> void Pool<T>::release( T *storage ) {
  assert( nullptr != storage );
  auto index = storage->pool_index;
  assert( index >= 0 && index < max_pool );
  auto &elem = list[index];
  assert( storage == elem.pobject );
  auto nindex = elem.next_index;
  auto pindex = elem.prev_index;
  // 加入到free_head链中
  elem.next_index = free_head;
  if ( free_head > bnf::notset ) {
    list[free_head].prev_index = index;
  }
  elem.prev_index = bnf::notset;
  free_head = index;

  // 从busy_head链中断开
  if ( nindex > bnf::notset ) {
    list[nindex].prev_index = pindex;
  }
  if ( pindex > bnf::notset ) {
    list[pindex].next_index = nindex;
  }
  if ( busy_head == index ) {
    busy_head = nindex;
  }

} // release

//----------------------------------------------------------------------------------------------
template <typename E> void Node_deleter( Node<E> *pNode ) {
  assert( nullptr != pNode );
  pNode->release();
}

template <typename E> using Node_ptr = std::unique_ptr<Node<E>, decltype( &Node_deleter<E> )>;

//------------------------------------------------------------------------------------------------
// 保存每次匹配结果,默认匹配是1次,可能是*,?,+,{min,max}指定次数的匹配
// 范围{0,-1}:*, {0,1}:?, {1,-1}:+
struct Range : Pool_item_t {
  // POD
  struct item_t {
    Position match_bi{ 0, 1, 0 };
    Position match_ei{ 0, 1, 0 };
    intptr_t result_index = 0;
    intptr_t result_bi = bnf::notset;
    intptr_t result_ei = bnf::notset;
  };
  constexpr static int16_t ARRSIZE = 64;
  constexpr static auto ARRINDEXDEFAULT = -1;
  constexpr static auto MAXARRINDEX = ARRSIZE - 1;
  item_t arr[ARRSIZE];
  int16_t arr_index = ARRINDEXDEFAULT;
  Range *prev = nullptr;
  Range *next = nullptr;
  Range *head = nullptr;
  Range *last = nullptr;

  bool pushed = false;
  using Pool_t = Pool<Range>;

public:
  static Range *create_header( Range::Pool_t &pool );

public:
  Range() = default;
  void release();
  void push_state( const item_t &item );
  void pop_back();
  item_t &get_first();
  item_t &get_last();
}; // Range

extern Range::Pool_t rangepool;

//----------------------------------------------------------------------------------------------
template <typename E> class Stack {
public:
  using node_t = Node<E>;
  constexpr static intptr_t PARSE_SN_BEGIN = 0x1 << 8;
  Stack() : parse_sn( PARSE_SN_BEGIN ){};
  void push_node( node_t *node, int parent_ni = FIRST_NODE_INDEX ) {
    auto nd = node->clone();
    assert( nd );
    auto size = node_list.size();
    nd->state = node_state::on_duty;
    nd->parent_index = parent_ni;
    nd->node_index = size;
    nd->parse_sn = ++parse_sn;
    nd->pSource = pSource;
    nd->pStack = this;
    nd->pResult = pResult;
    nd->hierarchy = compute_hierarchy( node, parent_ni );
    nd->range_state = Range::create_header( *rangepool );

    node_list.push_back( std::move( nd ) );
    statis_max_count = size > statis_max_count ? size : statis_max_count;
  }

  int compute_hierarchy( node_t *node, int parent_ni ) {
    auto hierarchy = 0;
    if ( parent_ni != FIRST_NODE_INDEX ) {
      assert( parent_ni < node_list.size() );
      auto hierarchy_p = node_list[parent_ni]->hierarchy;
      hierarchy = ( node->r_conf.hierarchy ? hierarchy_p + 1 : hierarchy_p );
    }
    return hierarchy;
  }

  template <typename NL> std::pair<int, int> push_product( NL &nl, int parent_ni ) {
    auto r = reverse( nl );
    auto count = nl.size();
    int product_bi = ( count == 0 ) ? bnf::notset : node_list.size();
    int product_ei = ( count == 0 ) ? bnf::notset : node_list.size() + count - 1;
    for ( auto &pNode : r ) {
      push_node( pNode, parent_ni );
    }
    if constexpr ( enable_statis_max_stack_node_count ) {
      if ( count > 0 && statis_node_count < product_ei + 1 ) {
        statis_node_count = product_bi + 1;
      }
    }
    return { product_bi, product_ei };
  }

  template <typename NL> std::pair<int, int> push_product( NL &nl, int parent_ni, int index ) {
    auto count = nl.size();
    assert( index >= 0 && index < count );
    int product_bi = bnf::notset;
    int product_ei = bnf::notset;
    if ( count > 0 ) {
      product_bi = node_list.size();
      product_ei = product_bi;
      push_node( nl[index], parent_ni );
      if constexpr ( enable_statis_max_stack_node_count ) {
        if ( count > 0 && product_ei + 1 > statis_node_count ) {
          statis_node_count = product_bi + 1;
        }
      }
    }
    return { product_bi, product_ei };
  }

  // collapse
  int collapse() {
    auto iter = find_if( node_list.rbegin(), node_list.rend(), []( auto &nd_ptr ) {
      return ( nullptr != nd_ptr ) ? ( nd_ptr->state & node_state::on_duty ) : true;
    } );
    auto count = iter - node_list.rbegin();
    node_list.erase( node_list.end() - count, node_list.end() );
    return count;
  }

  // latest node on_duty
  node_t *get_last_work() {
    auto iter = find_if( node_list.rbegin(), node_list.rend(), []( auto &nd_ptr ) {
      return ( nullptr != nd_ptr ) ? ( nd_ptr->state & node_state::on_duty ) : false;
    } );
    return ( iter == node_list.rend() ) ? nullptr : iter->get();
  }

  template <typename NODE = node_t> NODE *getNode( int index ) {
    assert( index < node_list.size() && 0 <= index );
    auto pNode = dynamic_cast<NODE *>( node_list[index].get() );
    assert( pNode != nullptr );
    return pNode;
  }

  // inform parent node
  void inform( int index, roadmap product_rdm ) {
    assert( 0 <= index && index < node_list.size() );
    node_list[index]->informed( product_rdm );
  }

  void offduty( intptr_t bi, intptr_t ei ) {
    assert( bi <= ei && bi >= 0 && ei >= 0 );
    int nd_count = node_count();
    if ( nd_count > 0 && bi < nd_count ) {
      if ( nd_count - 1 < ei ) {
        ei = nd_count - 1;
      }
      for ( int i = bi; i <= ei; ++i ) {
        assert( node_list[i] != nullptr );
        node_list[i]->offduty();
      }
    }
  }

  bool empty() {
    assert( node_list.size() > 0 && node_list[0].get() == nullptr );
    return node_list.size() == 1; // first node is not busy
  }

  void clean() { node_list.erase( node_list.begin() + 1, node_list.end() ); }

  int node_count() { return node_list.size(); }

public:
  Pool<Range> *rangepool;
  std::vector<Node_ptr<E>> node_list;
  Source<E> *pSource = nullptr;
  Result *pResult = nullptr;
  int parse_sn = 0; // serial number of nodes
  size_t statis_node_count = 0;
  size_t statis_max_count = 0;

}; // Stack

//----------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
struct match_info {
  int hierarchy;
  int parse_sn;
  cch *name;
  intptr_t bi;
  intptr_t ei = -1;
  cch *step;
  cch *terminal = "";
};

template <typename E> struct Node_base_pod {
  cch *name;
  int node_id;
  int range_min; // 最小匹配次数
  int range_max; // 最大匹配次数
  int matched_count;

  int state;
  intptr_t parent_index;
  intptr_t node_index;

  int parse_sn;   // 在一次parse过程中node被放到stack时给的唯一编号
  int parse_b_sn; // 开始匹配时最新的parse_sn
  int parse_e_sn; // 成功匹配时,保存匹配范围内的最后一个parse_sn
                  // 匹配失败时,从结果中清除从parse_being_sn到
                  // parse_e_sn的匹配结果
  roadmap match_result_last = roadmap::init;
  Range *range_state = nullptr;

  Position match_bi{ 0, 1, 0 }; // 当前被推导时匹配字符串的下标,在推导不成功时,
  Position match_ei{ 0, 1, 0 }; // 重新从这个下标开始匹配,范围[b,e)

  intptr_t product_bi = bnf::notset; // 侯选放到Stack时开始的下标
  intptr_t product_ei = bnf::notset; // 用于收缩时,将范围之内的Node都offduty

  int candidate_count;        // 侯选个数
  int candidate_count_remain; // 一轮匹配中还有多少个候选式仍未匹配

  int hierarchy = 0; // 层级

  ClassKind classkind = ClassKind::default_;

  int no_empty_scope = true;
  Result_conf r_conf;
  int32_t tokentype = 0;
  int epsilon = false;

  // enable_delay_product为真时, 所有候选式不是一次都放到栈中，
  // 而是根据实际匹配情况一次放一个,用这个来表示
  // 每轮匹配中放到栈中的候选产生式的下标。这样做可以略微提升性能
  int delay_product_index = 0;

  union {
    size_t extra_index = bnf::notset;
    void *extra_pointer; // 提供给Node添加额外信息的机会
  };

  Stack<E> *pStack = nullptr;   //
  Source<E> *pSource = nullptr; // match source
  Result *pResult = nullptr;

  RSWList<E> *rss_list = nullptr;
  size_t rss_index = bnf::notset;
}; // Node_base_pod

//------------------------------------------------------------------------------------------------
template <typename E> class Node : public Pool_item_t, public Node_base_pod<E> {
  friend Stack<E>;

public:
  Node( cch *t = "", int id = 0, int min = 1, int max = 1 ) {
    name = t;
    node_id = id;
    parent_index = 0;
    matched_count = 0;
    state = node_state::on_duty;
    range_min = min;
    range_max = max;
  }
  virtual ~Node() {}
  virtual int informed( roadmap product_rdm ) { return 0; } // for no product
  virtual roadmap match() = 0;
  virtual Node_ptr<E> clone() = 0;
  virtual void release() {
    if ( range_state ) {
      range_state->release();
      range_state = nullptr;
    }
  }
  virtual intptr_t push_result_range_result() = 0;
  virtual roadmap check_range( roadmap rdm ) {
    auto ret{ roadmap::unmatched };
    if ( rdm == roadmap::rangematched ) {
      ++matched_count;
      if ( range_min == 1 && range_max == 1 && matched_count == 1 ) {
        ret = roadmap::matched;
      } else {
        if ( range_max == bnf::notset || matched_count < range_max ) {
          ret = roadmap::matching;
        } else {
          if ( matched_count == range_max ) {
            ret = roadmap::matched;
          } else {
            assert( false );
          }
        }
      }
    } else {
      if ( rdm == roadmap::rangeunmatched ) {
        ret = roadmap::unmatched;
        if ( range_min == 0 || ( matched_count >= range_min && matched_count <= range_max ) ||
             ( range_max == bnf::notset && matched_count >= range_min ) ) {
          ret = ( matched_count == 0 ) ? roadmap::bypassmatched : roadmap::matched;
        }
      } else {
        assert( false );
      }
    }
    return ret;
  }

  virtual int inform_parent( roadmap product_rdm ) {
    if ( has_parent() ) {
      pStack->inform( parent_index, product_rdm );
    }
    return 0;
  }

  virtual int report_status( roadmap rdm ) {
    assert( rdm == roadmap::matched || rdm == roadmap::bypassmatched || rdm == roadmap::unmatched );
    offduty();
    inform_parent( rdm );
    return 0;
  }

  virtual void push_result_scope_begin() {
    if ( r_conf.visible && r_conf.hierarchy ) {
      auto &range_item = range_state->get_last();
      Result::item_t rslt;
      rslt.type = result_type::scope_begin;
      rslt.hierarchy = hierarchy;
      rslt.parent_parse_sn = 0;
      rslt.parent_node_id = 0;
      rslt.parse_sn = parse_sn;
      rslt.nid = node_id;
      rslt.classkind = classkind;
      rslt.tokentype = tokentype;
      rslt.name = name;
      rslt.bi = Position();
      rslt.ei = Position();
      range_item.result_bi = pResult->push_item( rslt );
    }
  }

  virtual void push_result_scope_end() {
    if ( r_conf.visible && r_conf.hierarchy ) {
      auto &range_item = range_state->get_last();
      if ( no_empty_scope && range_item.result_bi == pResult->max_index() ) {
        pResult->unwind( range_item.result_bi );
      } else {
        Result::item_t rslt;
        rslt.type = result_type::scope_end;
        rslt.hierarchy = hierarchy;
        rslt.parent_parse_sn = 0;
        rslt.parent_node_id = 0;
        rslt.parse_sn = parse_sn;
        rslt.nid = node_id;
        rslt.classkind = classkind;
        rslt.tokentype = tokentype;
        rslt.name = name;
        rslt.bi = Position();
        rslt.ei = Position();
        range_item.result_ei = pResult->push_item( rslt );
      }
    }
  }

  virtual void unwind_result() {
    if ( r_conf.visible && r_conf.hierarchy ) {
      auto &range_item = range_state->get_first();
      pResult->unwind( range_item.result_bi );
    }
  }

  virtual bool filter_result() { return true; }

public:
  void offduty() {
    if ( state & node_state::on_duty ) {
      state &= ~node_state::on_duty;
      offduty_product();
    }
  }

  void offduty_product() {
    if ( product_bi != bnf::notset && product_ei != bnf::notset ) {
      pStack->offduty( product_bi, product_ei );
    }
  }

  bool has_parent() {
    return parent_index >= 1; // first node is not busy
  }

  void push_range_status( const Range::item_t &range_item ) {
    assert( range_state );
    range_state->pushed = true;
    range_state->push_state( range_item );
  }

  void pop_range_state() {
    assert( range_state );
    if ( range_state->pushed ) {
      range_state->pop_back();
    }
  }

  bool in_pool() { return pool_index != bnf::notset; }

  bool compound() {
    return classkind == ClassKind::seque || classkind == ClassKind::alter;
  } // 当Nalter, Nseque时为真

  RSWItem<E> &get_rss() {
    assert( rss_list );
    auto &rsitem = rss_list->get_rss( rss_index );
    assert( rsitem.first );
    assert( rsitem.follow );
    return rsitem;
  }

public:
}; // Node

//------------------------------------------------------------------------------------------------
template <typename E> struct Candidate_list : std::vector<Node<E> *> {
  using super_t = std::vector<Node<E> *>;
  void push_clone_node( Node<E> &node ) {
    if ( node.in_pool() ) {
      this->push_back( &node );
    } else {
      auto nd = node.clone();          // Node_ptr
      this->push_back( nd.release() ); // keep it in pool
    }
  }
  void push_link_node( Node<E> &node ) {
    this->push_back( &node ); // just pointe to node
  }
  void pool_node() {
    assert( super_t::size() > 0 );
    for ( auto &p : self ) {
      if ( !p->in_pool() ) {
        auto nd = p->clone();
        p = nd.release();
      }
    }
  }
};

//----------------------------------------------------------------------------------------------
template <typename LIST> void recover_extra( const LIST &list ) {
  for ( auto &item : list ) {
    assert( item.pNode );
    item.pNode->extra_index = bnf::notset;
  }
}

//----------------------------------------------------------------------------------------------
// EE: Exclue Epsilon
enum class RoadSignType : int {
  rstFunc,
  rstVtChar,
  rstVtToken,
  rstEpsilon,
  rstNode = 0x10,
  rstFirstEE = 0x10 | 0x01,
  rstFollowEE = 0x10 | 0x02,
};
// using std::to_underlying in C++23
template <typename ENUM> auto to_underlying( ENUM e ) {
  return static_cast<std::underlying_type_t<ENUM>>( e );
}

union Symbol;
// 判断输入是否是可接受的终结符
// 返回结果roadmap::unmatched不成功, 或roadmap::matched表示成功
using RoadSignFunc = roadmap ( * )( const Symbol & );
union Symbol {
  uchar ch;                         // 词法分析
  Token token;                      // 语法分析
  void *node;                       // 用于求first和follow集过程中间状态
  RoadSignFunc is_acceptablevt = 0; // 判断终结符是否可被接受
};

struct RoadSign {
  RoadSignType type;
  Symbol symbol;

  bool operator<( const RoadSign &r ) const {
    auto rslt = type < r.type;
    if ( type == r.type ) {
      switch ( r.type ) {
        case RoadSignType::rstFunc: {
          rslt = r.symbol.is_acceptablevt < symbol.is_acceptablevt;
          break;
        }
        case RoadSignType::rstVtChar: {
          rslt = r.symbol.ch < symbol.ch;
          break;
        }
        case RoadSignType::rstVtToken: {
          rslt = r.symbol.token < symbol.token;
          break;
        }
        case RoadSignType::rstEpsilon: {
          // rslt = r.type < type;
          rslt = false;
          break;
        }
        case RoadSignType::rstFirstEE: {
          rslt = r.symbol.node < symbol.node;
          break;
        }
      }
    }
    return rslt;
  }
  RoadSign() = default;
  RoadSign( bnf::uchar ch ) {
    type = RoadSignType::rstVtChar;
    symbol.ch = ch;
  }
  RoadSign( bnf::Node_id nid ) {
    type = RoadSignType::rstVtToken;
    symbol.token.nid = nid;
  }
  RoadSign( bnf::Token token ) {
    type = RoadSignType::rstVtToken;
    symbol.token = token;
  }
  RoadSign( void *node, RoadSignType rst ) {
    assert( rst == RoadSignType::rstFirstEE || rst == RoadSignType::rstFollowEE );
    type = rst; //;
    symbol.node = node;
  }
}; // RoadSign

//----------------------------------------------------------------------------------------------
struct RoadSignSet : public std::set<RoadSign>, Pool_item_t {
  using super_t = std::set<RoadSign>;
  bool insert( const RoadSign &rs ) {
    // when overload functions provided, can't invoke super method directely
    return super_t::insert( rs ).second;
  }
  bool insert( bnf::uchar ch ) {
    RoadSign rs( ch );
    return super_t::insert( rs ).second;
  }
  bool insert( bnf::Node_id nid ) {
    RoadSign rs( nid );
    return super_t::insert( rs ).second;
  }
  bool insert( bnf::Token token ) {
    RoadSign rs( token );
    return super_t::insert( rs ).second;
  }
  bool insert( void *node, RoadSignType rst ) {
    RoadSign rs( node, rst );
    return super_t::insert( rs ).second;
  }
  bool union_exclue_epsilon( const RoadSignSet &r ) {
    auto ret = false;
    for ( auto &rs : r ) {
      if ( rs.type != RoadSignType::rstEpsilon ) {
        ret |= insert( rs );
      }
    }
    return ret;
  }
  bool exist( uchar ch ) {
    RoadSign rs( ch );
    auto found = self.find( rs ) != self.end();
    if ( !found ) {
      Symbol symbol;
      symbol.ch = ch;
      for ( auto &rs : self ) {
        if ( rs.type == RoadSignType::rstFunc ) {
          found |= rs.symbol.is_acceptablevt( symbol );
        }
        if ( found )
          break;
      }
    }
    return found;
  }
  bool exist( Token token ) {
    RoadSign rs( token );
    auto found = self.find( rs ) != self.end();
    if ( !found ) {
      Symbol symbol;
      symbol.token = token;
      for ( auto &rs : self ) {
        if ( rs.type == RoadSignType::rstFunc ) {
          found |= rs.symbol.is_acceptablevt( symbol );
        }
        if ( found )
          break;
      }
    }
    return found;
  }
  unsigned unexpand_roadsign_count = 0;
  template <class... Args> static shared_ptr<RoadSignSet> make( Args &&...args ) {
    return std::make_shared<RoadSignSet>( args... );
  }
}; // RoadSignSet

//----------------------------------------------------------------------------------------------
template <typename E> struct EpsilonItem {
  Node<E> *pNode = nullptr;
  bool epsilon = false;
  bool product = false;
  bool done = false;
};
template <typename E> using EpsilonList = std::vector<EpsilonItem<E>>;

//----------------------------------------------------------------------------------------------
template <typename E> bool mark_epsilon( bnf::Node<E> *p ) {
  using List = EpsilonList<E>;
  using Item = typename List::value_type;
  List list;
  {
    Item item;
    item.pNode = p;
    item.pNode->extra_index = list.size();
    list.push_back( item );
  }

  bool loop = true;
  while ( loop ) { // product all
    loop = false;
    auto currsize = list.size();
    for ( int64_t i = currsize - 1; i >= 0; --i ) {
      auto &citem = list[i];
      if ( !citem.done ) { // access current item by list[i]
        assert( citem.pNode );
        ////dbg(i, list[i].pNode->name, list[i].product, list[i].pNode->compound());
        if ( !citem.product && citem.pNode->compound() ) {
          auto cmpd = dynamic_cast<Ncompound<E> *>( citem.pNode );
          assert( cmpd );
          citem.product = true;
          for ( auto &cnode : cmpd->get_cddlist() ) {
            assert( cnode && cnode->pool_object );
            if ( cnode->extra_index == bnf::notset ) {
              assert( cnode->classkind != ClassKind::default_ );
              Item item;
              item.pNode = cnode;
              if ( cnode->classkind == ClassKind::func ) {
                assert( cnode->range_min > 0 );
                item.epsilon = false;
                item.product = true;
                item.done = true;
              } else {
                assert( cnode->compound() );
                item.epsilon = ( cnode->range_min == 0 );
                item.product = false;
                item.done = false;
              }
              item.pNode->extra_index = list.size();
              list.push_back( item );
              loop = true;
              ////dbg(pNode->name);
            }
          }
        }
      }
    }
  }

  // dbg( list.size() );
  //_getwch();

  loop = true;
  while ( loop ) {
    loop = false;
    auto currsize = list.size();
    for ( int64_t i = currsize - 1; i >= 0; --i ) { // signed
      auto &citem = list[i];                        // list size no more changed
      if ( !citem.done ) {
        if ( citem.pNode->compound() ) {
          assert( citem.product );
          auto cmpd = dynamic_cast<Ncompound<E> *>( citem.pNode );
          if ( citem.epsilon ) { // range_min==0
            citem.done = true;
          }
          if ( !citem.done ) {
            if ( citem.pNode->classkind == ClassKind::seque ) {
              bool epsilon = true;
              for ( auto &pNode : cmpd->get_cddlist() ) {
                auto &cdd = list[pNode->extra_index];
                if ( cdd.done ) {
                  if ( !cdd.epsilon ) {
                    epsilon = false;
                    citem.done = true; // citem.epsilon==false by default
                    loop = true;
                    break;
                  } // else check next
                } else {
                  if ( !cdd.epsilon ) {
                    epsilon = false;
                    break;
                  } // else check next
                }
              }
              if ( epsilon ) {
                citem.epsilon = true;
                citem.done = true;
                loop = true;
              }
            } // Nseque

            if ( citem.pNode->classkind == ClassKind::alter ) {
              bool epsilon = false;
              for ( auto &pNode : cmpd->get_cddlist() ) {
                auto &cdd = list[pNode->extra_index];
                if ( cdd.epsilon ) {
                  epsilon = true;
                  break;
                }
              }
              if ( epsilon ) {
                citem.done = true;
                citem.epsilon = true;
                loop = true;
              } else {
                bool done = true;
                for ( auto &pNode : cmpd->get_cddlist() ) {
                  auto &cdd = list[pNode->extra_index];
                  done = done && cdd.done;
                }
                if ( done ) {
                  citem.done = true;
                  loop = true;
                } // wait candidate done
              }
            } // Nalter
          }
        }
      }
    }
  }

  // dbg( "-----------------------" );
  auto epsilon_count = 0;
  for ( auto &item : list ) {
    if ( item.epsilon ) {
      assert( item.pNode );
      item.pNode->epsilon = true;
      // dbg( item.pNode->name );
      ++epsilon_count;
    }
  }
  // dbg( epsilon_count );
  //_getwch();

  // dbg( "-----------------------" );
  auto undone_count = 0;
  for ( auto &item : list ) {
    if ( !item.done ) {
      dbg( item.pNode->name );
      ++undone_count;
    }
  }
  assert( 0 == undone_count );
  // dbg( undone_count );
  //_getwch();

  recover_extra( list );

  //_getwch();
  return 0 == undone_count;
}

//----------------------------------------------------------------------------------------------
template <typename E> struct RSWItem {
  Node<E> *pNode = nullptr;
  shared_ptr<RoadSignSet> first;
  shared_ptr<RoadSignSet> follow;
  bool done = false;
}; // road sign work item

template <typename E> struct RSWList : public std::vector<RSWItem<E>> {
  template <typename E> struct RSWReminder { Node<E> *pNode = nullptr; };
  template <typename E> using RSWReminderV = std::vector<RSWReminder<E>>;

  EpsilonList<E> elist;

  using Item = typename RSWList<E>::value_type;

  auto &get_rss( size_t index ) {
    assert( index >= 0 && index < self.size() );
    return self[index];
  }

  auto get_node_roadsignset( void *node, RoadSignType rst, size_t sz ) {
    RoadSignSet *prss = nullptr;
    auto pNode = reinterpret_cast<Node<E> *>( node );
    assert( pNode );
    auto index = pNode->extra_index;
    assert( index >= 0 && index < sz );
    auto &rel_item = self[index];
    // dbg( rel_item.pNode->name );
    switch ( rst ) {
      case RoadSignType::rstFirstEE: prss = rel_item.first.get(); break;
      case RoadSignType::rstFollowEE: prss = rel_item.follow.get(); break;
    }
    assert( prss );
    return prss;
  }

  bool expand_roadsign( shared_ptr<RoadSignSet> &rss, size_t sz ) {
    auto expanded = false;
    // XXX: do not use range_based for loop
    for ( auto iter = rss->begin(); iter != rss->end(); ++iter ) {
      if ( to_underlying( iter->type ) & to_underlying( RoadSignType::rstNode ) ) {
        auto prss = get_node_roadsignset( iter->symbol.node, iter->type, sz );
        if ( prss->unexpand_roadsign_count == 0 ) {
          rss->erase( iter );
          rss->union_exclue_epsilon( *prss );
          rss->unexpand_roadsign_count -= 1;
          expanded = true; // since have changed unexpand_roadsign_count
          break;           // just expand one
        }
      }
    }
    return expanded;
  }

  bool insert_another_first_ee( shared_ptr<RoadSignSet> &dest, void *node ) {
    assert( dest );
    auto ok = dest->insert( node, RoadSignType::rstFirstEE );
    dest->unexpand_roadsign_count += ok ? 1 : 0;
    return ok;
  }

  bool insert_another_follow_ee( shared_ptr<RoadSignSet> &dest, void *node ) {
    assert( dest );
    auto ok = dest->insert( node, RoadSignType::rstFollowEE );
    dest->unexpand_roadsign_count += ok ? 1 : 0;
    return ok;
  }

  void first_loop() {
    auto loop = true;
    while ( loop ) {
      loop = false;
      for ( auto &item : self ) {
        assert( item.pNode );
        switch ( item.pNode->classkind ) {
          case ClassKind::seque: {
            auto cmpd = dynamic_cast<Ncompound<E> *>( item.pNode );
            assert( cmpd );
            for ( auto &cdd : cmpd->get_cddlist() ) {
              loop |= self.insert_another_first_ee( item.first, cdd );
              // dbg( loop, cmpd->name, cdd->name, item.first.unexpand_roadsign_count );
              // dbg( cmpd->name, cdd->name, loop );
              if ( !cdd->epsilon ) {
                break;
              }
            }
          } break;
          case ClassKind::alter: {
            auto cmpd = dynamic_cast<Ncompound<E> *>( item.pNode );
            assert( cmpd );
            for ( auto &cdd : cmpd->get_cddlist() ) {
              loop |= self.insert_another_first_ee( item.first, cdd );
              // dbg( cmpd->name, cdd->name );
            }
          } break;
        }
      }
    }
  }

  void follow_loop() {
    auto loop = true;
    while ( loop ) {
      loop = false;
      for ( auto &item : self ) {
        assert( item.pNode );
        switch ( item.pNode->classkind ) {
          case ClassKind::seque: {
            auto cmpd = dynamic_cast<Ncompound<E> *>( item.pNode );
            assert( cmpd );
            auto &cdd_list = cmpd->get_cddlist();
            auto cdd_sz = cdd_list.size();
            for ( auto i = 0; i < cdd_sz; ++i ) {
              auto &cdd = cdd_list[i];
              auto &cdd_item = self[cdd->extra_index];
              if ( cdd->compound() ) {   // 只针对非终结符
                if ( i == cdd_sz - 1 ) { // A --> xyzB
                  loop |= self.insert_another_follow_ee( cdd_item.follow, item.pNode );
                } else {
                  auto j = i + 1;
                  for ( ; j < cdd_sz; ++j ) { // A-->xBy
                    auto cdd_j = cdd_list[j];
                    loop |= self.insert_another_first_ee( cdd_item.follow, cdd_j );
                    if ( !cdd_j->epsilon ) {
                      break;
                    }
                  }
                  if ( j == cdd_sz ) { // A-->xByz (yz-->epsilon)
                    loop |= self.insert_another_follow_ee( cdd_item.follow, item.pNode );
                  }
                }
              } // else Nfunc
            }
          } break;
          case ClassKind::alter: { // A--> B | C | D
            auto cmpd = dynamic_cast<Ncompound<E> *>( item.pNode );
            assert( cmpd );
            for ( auto &cdd : cmpd->get_cddlist() ) {
              if ( cdd->compound() ) {
                auto &cdd_item = self[cdd->extra_index];
                loop |= self.insert_another_follow_ee( cdd_item.follow, item.pNode );
              }
            }
          } break;
        }
      }
    }
  }

  void expand_rss() {
    auto loop = true;
    auto sz = self.size();
    while ( loop ) {
      loop = false;
      for ( auto &item : self ) {
        if ( item.first->unexpand_roadsign_count > 0 ) {
          loop |= self.expand_roadsign( item.first, sz );
        }
        if ( item.follow->unexpand_roadsign_count > 0 ) {
          loop |= self.expand_roadsign( item.follow, sz );
        }
      }
    }
    auto unexpand_node_count = 0;
    // dbg( "-----------------------" );
    for ( auto &item : self ) {
      if ( item.pNode->compound() ) {
        if ( item.first->unexpand_roadsign_count > 0 ) {
          dbg( item.pNode->name );
          for ( auto &rs : *item.first ) {
            if ( rs.type > RoadSignType::rstNode ) {
              auto pNode = reinterpret_cast<Node<E> *>( rs.symbol.node );
              dbg( rs.type, pNode->name );
            }
          }
          dbg( "" );
          ++unexpand_node_count;
        }
      }
    }
    // dbg( unexpand_node_count );
    // dbg( "-----------------------" );
    assert( unexpand_node_count == 0 );
    // for ( auto &item : self ) {
    // dbg( item.pNode->name );
    // dbg( item.first->size() );
    // dbg( item.follow->size() );
    //}
    // dbg( "" );
  }

  void list_one_node_product( const RSWReminder<E> &r, RSWReminderV<E> &rmdr ) {
    assert( r.pNode );
    switch ( r.pNode->classkind ) {
      case ClassKind::alter: [[fallthrough]];
      case ClassKind::seque: {
        auto cmpd = dynamic_cast<Ncompound<E> *>( r.pNode );
        assert( cmpd );
        Item item;
        item.pNode = r.pNode;
        item.pNode->extra_index = self.size(); // use extra_index keep index of item
        item.first = RoadSignSet::make();
        item.follow = RoadSignSet::make();
        self.push_back( item );
        auto &cddlist = cmpd->get_cddlist();
        assert( cddlist.size() > 0 );
        for ( auto &cnode : cddlist ) {
          assert( cnode );
          if ( cnode->extra_index == bnf::notset ) {
            rmdr.push_back( { cnode } );
          }
        }
        break;
      }
      default: { // Nfunc函数代表终结符函数
        assert( r.pNode->classkind == ClassKind::func );
        auto nfunc = dynamic_cast<Nfunc<E> *>( r.pNode );
        assert( nfunc );
        Item item;
        item.pNode = r.pNode;
        item.pNode->extra_index = self.size(); // use extra_index keep index of item
        item.first = RoadSignSet::make( nfunc->getFirst_list() );
        item.follow = RoadSignSet::make();
        assert( item.first );
        assert( item.follow );
        item.done = true;
        self.push_back( item );
        break;
      }
    }
  }

  void list_products( bnf::Node<E> *p ) {
    assert( p );
    RSWReminderV<E> rmd{ { p } };
    auto loop = true;
    while ( loop ) {
      loop = false;
      for ( auto r = rmd.rbegin(); r != rmd.rend(); ++r ) {
        assert( r->pNode );
        auto &node = *r->pNode;
        loop = ( node.extra_index == bnf::notset );
        if ( loop ) {
          // dbg(r->pNode->name);
          list_one_node_product( *r, rmd );

          // set RoadSignList and item index for node
          node.rss_list = this;
          node.rss_index = node.extra_index;
          break;
        }
      }
    }
  }

  void collect_roadsign( bnf::Node<E> *p ) {
    mark_epsilon( p );
    list_products( p );
    first_loop();
    follow_loop();
    expand_rss();
    recover_extra( self );
  }

}; // RSWList

//----------------------------------------------------------------------------------------------
#define POOL_TREE( node ) dynamic_cast<decltype( node ) *>( bnf::pool_tree( node ) )

//----------------------------------------------------------------------------------------------
template <typename E> bnf::Node<E> *pool_tree( bnf::Node<E> &node ) {
  struct PoolWorkItem {
    Node<E> *pnode = nullptr; // in pool or not
    intptr_t pool_index = bnf::notset;
    Node<E> *node_in_pool = nullptr; // 用于快速定位（当node被pool时，用这个来保存其地址）
    bool done = false;               // 自身及其推导都被pool之后为真
    Node<E> **node_pp = nullptr;
  };
  std::vector<PoolWorkItem> checklist;
  std::set<Node<E> *> node_extra_set;
  PoolWorkItem ne;
  ne.pnode = &node;
  checklist.push_back( {} ); // index==0 to stop
  checklist.push_back( ne );
  uint64_t index = 1;
  ////dbg( node.name );
  while ( index ) {
    auto &item = checklist[index];
    if ( item.done ) {
      --index;
    } else {
      item.done = true;
      auto have_same_preitem = false;
      assert( item.pnode );
      if ( item.pool_index == bnf::notset ) {
        if ( item.pnode->in_pool() ) {
          item.node_in_pool = item.pnode;
          item.pool_index = item.pnode->pool_index;
        } else {
          if ( item.pnode->extra_index != bnf::notset ) {
            // 当候选式中有多个指向同一个node时并且这个node不是pool中，则node首次pool时会更忙extra_index
            // 来表示已经被pool,这样根据extra_index来判断是否node被pool,后续的pool则无须再pool一次
            // 使用前面已经pool的同一node在checklist中的信息来更新当前item
            auto index_of_node = item.pnode->extra_index;
            auto &preitem = checklist[index_of_node];
            item.node_in_pool = preitem.node_in_pool;
            item.pool_index = preitem.pool_index;
            have_same_preitem = true;
            ////dbg( item.node_in_pool->name );
          } else {
            // 如果pnode不在pool中，而且也没有前导被pool,则pool它
            item.node_in_pool = item.pnode->clone().release();
            item.pool_index = item.node_in_pool->pool_index;
            ////dbg( item.node_in_pool->name );
          }
        }
        item.pnode->extra_index = item.node_in_pool->extra_index = index;
        node_extra_set.insert( item.pnode );
        node_extra_set.insert( item.node_in_pool );
        if ( item.node_pp ) {
          *item.node_pp = item.node_in_pool;
        }
      } // else means item's node_in_pool in pool

      assert( item.node_in_pool );
      if ( item.node_in_pool->compound() && !have_same_preitem ) {
        auto compound = dynamic_cast<Ncompound<E> *>( item.node_in_pool );
        assert( compound );
        auto &node_in_pool = *compound;
        node_in_pool.pool_candidate_flag = true;
        for ( auto &p_to_candidate : *node_in_pool.candidate_list ) {
          auto index_of_node = (int64_t)p_to_candidate->extra_index;
          if ( index_of_node != bnf::notset ) { // node in checklist
            assert( index_of_node > 0 && index_of_node < checklist.size() );
            // node already in pool, just update pointer to it
            p_to_candidate = checklist[index_of_node].node_in_pool;
          } else {
            PoolWorkItem ne;
            ne.pnode = p_to_candidate;
            // save current node's address, when candidate node pooled
            // update it's node_in_pool address to this address
            ne.node_pp = &p_to_candidate;
            checklist.push_back( ne ); // 可能导致item失效
          }
        }
        index = checklist.size() - 1;
      }
    }
  } // while ( index )
  ////dbg( "}}}" );

  // XXX: recover extra_index setting
  for ( auto &p : node_extra_set ) {
    p->extra_index = bnf::notset;
  }
  assert( checklist.size() >= 2 && checklist[1].node_in_pool );
  return checklist[1].node_in_pool;
}

//----------------------------------------------------------------------------------------------

template <typename E> class Ncompound : public Node<E> {
  using super_t = Node<E>;

public:
  Ncompound( cch *t = "", int id = 0, int min = 1, int max = 1 ) : super_t( t, id, min, max ) {
    candidate_list = std::make_shared<Candidate_list<E>>();
  }

  void pool_candidate() {
    assert( pool_candidate_flag == false );
    candidate_list->pool_node();
    pool_candidate_flag = true;
  }

  Nalter<E> &asNalter( int index ) {
    assert( index >= 0 && index < candidate_list->size() );
    return *reinterpret_cast<Nalter<E> *>( ( *candidate_list )[index] );
  }

  Nseque<E> &asNseque( int index ) {
    assert( index >= 0 && index < candidate_list.size() );
    return *reinterpret_cast<Nseque<E> *>( ( *candidate_list )[index] );
  }

  Candidate_list<E> &get_cddlist() {
    auto p = candidate_list.get();
    assert( p );
    return ( *p );
  }

  size_t inquiry_branchs( const E &elem ) {
    size_t branchs_count = 0;
    switch ( classkind ) {
      case ClassKind::alter: {
        size_t index = 0;
        for ( auto &cdd : *candidate_list ) {
          auto &item = cdd->get_rss();
          if ( item.first->exist( elem ) ) {
            rs_branchs[++branchs_count] = index;
          } else {
            if ( cdd->epsilon ) {
              assert( item.follow );
              if ( item.follow->exist( elem ) ) {
                rs_branchs[++branchs_count] = index;
              }
            }
          }
          ++index;
        }
        break;
      }
      case ClassKind::seque: {
        // do nothing, since one branch fro seque node
        break;
      }
    }
    assert( branchs_count > 0 );
    rs_branchs[0] = branchs_count; // first item is count
    return branchs_count;
  }

public:
  // pool_candidate_flag default false, using pool_tree to pool all link node
  // or pool_candidate just pool candidate itself
  bool pool_candidate_flag = false;

  shared_ptr<Candidate_list<E>> candidate_list;
  unsigned rs_branchs[max_static_roadsign_branch];
}; // Ncompound

//----------------------------------------------------------------------------------------------
// 只要有一个推导匹配成功则这个Node匹配成功
template <typename E> class Nalter : public Ncompound<E> {
  using self_t = Nalter;
  using ref_t = Nalter &;
  using nodeptr_t = Node_ptr<E>;
  using node_t = Node<E>;
  using Pool_t = Pool<Nalter>;

public:
  Nalter( cch *t = "", int id = 0, int min = 1, int max = 1 ) : Ncompound<E>( t, id, min, max ) {
    classkind = ClassKind::alter;
  }
  virtual int informed( roadmap product_rdm ) override {
    switch ( product_rdm ) {
      case roadmap::bypassmatched: [[fallthrough]];
      case roadmap::matched: {
        pSource->getposition( range_state->get_last().match_ei );
        offduty_product();
        match_result_last = roadmap::rangematched;
        break;
      }
      case roadmap::unmatched: {
        pSource->setposition( range_state->get_last().match_bi );
        ////dbg( candidate_count );
        if ( --candidate_count_remain <= 0 ) {
          offduty_product();
          match_result_last = roadmap::rangeunmatched;
        } // else nothing needs to be changed, try matching another candidate
        break;
      }
    }
    return 0;
  }
  //// virtual int report_status(roadmap) override;
  virtual roadmap match() override {
    auto ret{ roadmap::matching };

    ////dbg( match_result_last );
    if ( match_result_last != roadmap::init ) {
      ret = match_result_last;
      // 再次做相关变量的初始化
      match_result_last = roadmap::init;
      parse_sn = ++pStack->parse_sn;
      if constexpr ( enable_delay_product ) {
        delay_product_index = 0;
      }
    } else {
      if constexpr ( enable_delay_product ) {
        if ( delay_product_index == 0 ) {
          // start new turn
          E elem;
          pSource->peek( elem ); // FIXME: when return is false
          candidate_count = inquiry_branchs( elem );
          candidate_count_remain = candidate_count;
          assert( candidate_count > 0 );

          pSource->getposition( match_bi );
          push_range_status( { match_bi } );
          push_result_scope_begin();
        }

        assert( delay_product_index <= candidate_count ); // delay_product_index 从1开始
        auto cdd_index = rs_branchs[++delay_product_index];
        std::tie( product_bi, product_ei ) =
            pStack->push_product( *candidate_list, node_index, cdd_index );

        ret = roadmap::producted;
      } else {
        pSource->getposition( match_bi );
        push_range_status( { match_bi } );
        push_result_scope_begin();

        std::tie( product_bi, product_ei ) = pStack->push_product( *candidate_list, node_index );
        candidate_count = candidate_list->size();
        candidate_count_remain = candidate_count;
        assert( candidate_count > 0 );

        ret = roadmap::producted;
      }
    }

    return ret;
  }

  virtual Node_ptr<E> clone() override {
    static Pool<Nalter> pool;
    return { pool.clone( self ), Node_deleter<E> };
  }

  virtual void release() override {
    assert( pool_object );
    Node::release();
    reinterpret_cast<Pool_t *>( pool_object )->release( this );
  }

  virtual intptr_t push_result_range_result() override {
    intptr_t index = 0;
    return index;
  }

  template <typename NODE, typename... Tail> ref_t operator()( NODE &node, Tail &...tail ) {
    if ( pool_candidate_flag ) {
      candidate_list->push_clone_node( node );
    } else {
      candidate_list->push_link_node( node );
    }
    if constexpr ( sizeof...( Tail ) > 0 ) {
      this->operator()( tail... );
    }
    return self;
  }

}; // Nalter

//----------------------------------------------------------------------------------------------
// 必须所有推导匹配成功才能使得这个Node匹配成功
template <typename E> class Nseque : public Ncompound<E> {
  using self_t = Nseque;
  using ref_t = Nseque &;

public:
  Nseque( cch *t = "", int id = 0, int min = 1, int max = 1 ) : Ncompound<E>( t, id, min, max ) {
    classkind = ClassKind::seque;
  }
  virtual ~Nseque() {}
  virtual int informed( roadmap product_rdm ) override {
    switch ( product_rdm ) {
      case roadmap::bypassmatched: [[fallthrough]];
      case roadmap::matched: {
        pSource->getposition( range_state->get_last().match_ei );
        if ( --candidate_count_remain <= 0 ) {
          offduty_product();
          match_result_last = roadmap::rangematched;
        }
        break;
      }

      case roadmap::unmatched: {
        pSource->setposition( range_state->get_last().match_bi );
        offduty_product();
        match_result_last = roadmap::rangeunmatched;
        break;
      }
    }

    return 0;
  }
  // virtual int report_status(roadmap) override;
  virtual roadmap match() override {
    auto ret{ roadmap::matching };

    if ( match_result_last != roadmap::init ) {
      ret = match_result_last;
      match_result_last = roadmap::init;
      parse_sn = ++pStack->parse_sn;
      if constexpr ( enable_delay_product ) {
        delay_product_index = 0;
      }
    } else {
      if constexpr ( enable_delay_product ) {
        if ( delay_product_index == 0 ) {
          candidate_count = candidate_list->size();
          candidate_count_remain = candidate_count;
          assert( candidate_count > 0 );

          pSource->getposition( match_bi );
          push_range_status( { match_bi } );
          push_result_scope_begin();
        }

        assert( delay_product_index < candidate_count );
        std::tie( product_bi, product_ei ) =
            pStack->push_product( *candidate_list, node_index, delay_product_index );
        ++delay_product_index;

        ret = roadmap::producted;
      } else {
        pSource->getposition( match_bi );
        push_range_status( { match_bi } );
        push_result_scope_begin();

        std::tie( product_bi, product_ei ) = pStack->push_product( *candidate_list, node_index );
        candidate_count = candidate_list->size();
        candidate_count_remain = candidate_count;
        assert( candidate_count > 0 );
        ret = roadmap::producted;
      }
    }

    return ret;
  }
  virtual Node_ptr<E> clone() override {
    static Pool<Nseque> pool;
    return { pool.clone( self ), Node_deleter<E> };
  }

  virtual void release() override {
    assert( pool_object );
    Node::release();
    reinterpret_cast<Pool<Nseque> *>( pool_object )->release( this );
  }

  virtual intptr_t push_result_range_result() override {
    intptr_t index = 0;
    return index;
  }

  template <typename NODE, typename... Tail> ref_t operator()( NODE &node, Tail &...tail ) {
    if ( pool_candidate_flag ) {
      candidate_list->push_clone_node( node );
    } else {
      candidate_list->push_link_node( node );
    }
    if constexpr ( sizeof...( Tail ) > 0 ) {
      this->operator()( tail... );
    }
    return self;
  }

}; // Nseque

//----------------------------------------------------------------------------------------------
template <typename E> using node_f = roadmap ( * )( Nfunc<E> &node );

//----------------------------------------------------------------------------------------------
template <typename E> class Nfunc : public Node<E> {
  using self_t = Nfunc;

public:
  using RSSPool = Pool<RoadSignSet>;
  static RSSPool &getRSSPool() {
    static RSSPool pool;
    return pool;
  }
  static void clearRSSPool() { getRSSPool().clear(); }

public:
  node_f<E> match_func = nullptr;
  RSSPool *rss_pool = nullptr;
  void *data = nullptr;

  RSSPool::value_t &getFirst_list() {
    assert( first_list );
    return *first_list;
  }

protected:
  RSSPool::value_t *first_list = nullptr;

protected:
  Nfunc( cch *t, node_f<E> mf, int id = 0, int min = 1, int max = 1, void *data_pv = nullptr )
      : Node<E>( t, id, min, max ), match_func( mf ), data( data_pv ) {
    classkind = ClassKind::func;
    assert( min >= 1 && max >= min );
    assert( match_func );
    rss_pool = &Nfunc::getRSSPool();
    first_list = rss_pool->alloc();
  }

public:
  Nfunc( cch *t, node_f<E> mf, RoadSignFunc rsf, int id = 0, int min = 1, int max = 1,
         void *data_pv = nullptr )
      : Nfunc{ t, mf, id, min, max, data_pv } {
    classkind = ClassKind::func;
    assert( min >= 1 && max >= min );

    RoadSign rs;
    rs.type = RoadSignType::rstFunc;
    rs.symbol.is_acceptablevt = rsf;
    first_list->insert( rs );
  }

  // Nfunc( cch *t, node_f<E> mf, const std::vector<RoadSign> &rss, int id = 0, int min = 1,
  // int max = 1, void *data_pv = nullptr )
  //: Nfunc{ t, mf, id, min, max, data_pv } {
  // classkind = ClassKind::func;
  // assert( min >= 1 && max >= min );

  // for ( const auto &rs : rss ) {
  // first_list->insert( rs );
  //}
  //}

  Nfunc( cch *t, node_f<E> mf, const std::initializer_list<RoadSign> &&rss, int id = 0, int min = 1,
         int max = 1, void *data_pv = nullptr )
      : Nfunc{ t, mf, id, min, max, data_pv } {
    classkind = ClassKind::func;
    assert( min >= 1 && max >= min );

    for ( const auto &rs : rss ) {
      first_list->insert( rs );
    }
  }

  // virtual int report_status(roadmap) override;
  virtual roadmap match() override {
    auto ret{ roadmap::matching };

    if ( match_result_last != roadmap::init ) {
      ret = match_result_last;
      match_result_last = roadmap::init;
      parse_sn = ++pStack->parse_sn;
    } else {
      pSource->getposition( match_bi );
      push_range_status( { match_bi } );
      push_result_scope_begin();

      // match_result_last = match_func( self );
      assert( match_func );
      assert( first_list && first_list->size() > 0 );
      match_result_last = match_func( self );
      assert( match_result_last == roadmap::matched || match_result_last == roadmap::unmatched );

      if ( match_result_last == roadmap::matched ) {
        match_result_last = roadmap::rangematched;
        range_state->get_last().result_index = push_result_range_result();
      } else {
        match_result_last = roadmap::rangeunmatched;
      }

      ret = match_result_last;
    }

    return ret;
  }

  virtual Node_ptr<E> clone() override {
    static Pool<self_t> pool;
    return { pool.clone( self ), Node_deleter<E> };
  }

  virtual void release() override {
    assert( pool_object );
    Node::release();
    reinterpret_cast<Pool<self_t> *>( pool_object )->release( this );
  }

  virtual intptr_t push_result_range_result() override {
    intptr_t index = 0;
    if ( r_conf.visible ) {
      assert( match_result_last == roadmap::rangematched );
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

}; // Nfunc

//----------------------------------------------------------------------------------------------
template <auto nid, auto AID> roadmap kw_f( Sfunc &node ) {
  auto rdm = roadmap::unmatched;
  assert( node.pSource );
  auto &source = *node.pSource;
  auto tk = Token{};
  if ( source.get( tk ) ) {
    switch ( tk.nid ) {
      case nid:
        rdm = roadmap::matched;
        node.node_id = AID;
        break;
    }
  }
  if ( rdm == roadmap::unmatched ) {
    source.setposition( node.match_bi );
  }
  return rdm;
}

template <auto nid> roadmap kw_frs( const Symbol &symbol ) {
  return ( symbol.token.nid == nid ) ? roadmap::matched : roadmap::unmatched;
}

//----------------------------------------------------------------------------------------------
template <auto NID, auto AID = NID> struct Tfunc : public Nfunc<Token> {
  Tfunc( cch *t, int min = 1, int max = 1, void *data_pv = nullptr )
      : Nfunc( t, kw_f<NID, AID>, { RoadSign{ (Node_id)NID } }, AID, min, max, data_pv ) {}
};

//----------------------------------------------------------------------------------------------
// extern Nfunc_partner<uchar> SPACE_FP;
// extern Nfunc_partner<uchar> ID_FP;
// extern Nfunc_partner<uchar> SPACE_FP;
//----------------------------------------------------------------------------------------------
roadmap SPACE_F( Nfunc<uchar> &node );
roadmap SPACE_FRS( const Symbol &symbol );

roadmap ID_F( Nfunc<uchar> &node );
roadmap ID_FRS( const Symbol &symbol );

roadmap SLASHSTRING_F( Nfunc<uchar> &node );
roadmap SLASHSTRING_FRS( const Symbol &symbol );

roadmap QUOTESTRING_F( Nfunc<uchar> &node );
roadmap QUOTESTRING_FRS( const Symbol &symbol );

roadmap DQUOTESTRING_F( Nfunc<uchar> &node );
roadmap DQUOTESTRING_FRS( const Symbol &symbol );

roadmap REALNUMBER_F( Nfunc<uchar> &node );
roadmap REALNUMBER_FRS( const Symbol &symbol );

roadmap REALNUMBER_NS_F( Nfunc<uchar> &node ); // no signed
roadmap REALNUMBER_NS_FRS( const Symbol &symbol );

//----------------------------------------------------------------------------------------------
using Vpchs = std::vector<cch *>;
//----------------------------------------------------------------------------------------------
// 匹配一个字符串
class Nliteral : public Nfunc<uchar> {
  using self_t = Nliteral;
  using Pool_t = Pool<Nliteral>;

public:
  Nliteral( cch *t, cch *s, int id = 0, int min = 1, int max = 1 );
  virtual roadmap match() override;
  virtual Node_ptr<uchar> clone() override;
  virtual void release() override;
  virtual intptr_t push_result_range_result() override;

  static roadmap match_func( Nfunc<uchar> &node );

protected:
  cch *literal_str;
  int str_len = 0;

}; // Nliteral

//----------------------------------------------------------------------------------------------
struct MatchCursor {
  enum MCTYPE {
    mcMatched,
    mcUnmatched,
  };
  bool used = false;
  cch *node_name = "";
  Position match_bi = Position();
  MCTYPE kind = mcMatched;

  template <typename SOURCE> void print( SOURCE &src ) {
    auto& pos = match_bi;
    if ( pos.index >= 0 && pos.index < src.size() ) {
      printf( R"(
match_cursor = {
  active=%s,
  kind='%s',
  node_name='%s',
  source={size=%lld},
  pos={index=%lld, line=%lld, column=%lld},
}
)",
              used ? "on" : "off", kind == mcMatched ? "matched" : "unmatch", node_name, src.size(),
              pos.index, pos.linenum, pos.col );
    } else {
      printf( R"(
match_cursor = {
  active=%s,
  kind='%s',
  node_name='%s',
  source={size=%lld},
  pos={index=%lld, line=%lld, column=%lld},
}
)",
              used ? "on" : "off", kind == mcMatched ? "matched" : "unmatch", node_name, src.size(),
              pos.index, pos.linenum, pos.col );
    }
  }
};

//----------------------------------------------------------------------------------------------
template <typename E> class Parser {
public:
  Parser( Source<E> &source, Stack<E> &stack, Result &result ) : pRangepool( &rangepool ) {
    pSource = &source;
    pResult = &result;
    pStack = &stack;
    pSource->statis_get = 0;
    pStack->pResult = pResult;
    pStack->pSource = pSource;
    pStack->rangepool = pRangepool;
    last_unmatch.kind = MatchCursor::MCTYPE::mcUnmatched;
  }

  void print_matchcursor() {
    assert( pResult );
    last_match.print( pResult->data );
    last_unmatch.print( pResult->data );
  }

  void prepare( Node<E> *first_node ) {
    assert( first_node );
    pResult->data.clear();
    pStack->node_list.clear();
    pStack->node_list.push_back( { nullptr, Node_deleter<E> } ); // first node is not busy
    pStack->push_node( first_node );

    rslist.collect_roadsign( first_node );
  }

  roadmap parse( Node<E> *first_node ) {
    prepare( first_node );

    constexpr auto MAX_LOOP_COUNT = 100000000;
    rdm = roadmap::matching;
    loop_count = 0;
    while ( rdm != roadmap::fail && rdm != roadmap::success && ( ++loop_count < MAX_LOOP_COUNT ) ) {
      // dbg( "\n", loop_count );
      // dbg(rdm);
      switch ( rdm ) {
        case roadmap::matching: {
          // dbg( "case matching" );
          auto pNode = pStack->get_last_work();
          if ( pNode ) {
            // dbg( pNode->name );
            rdm = pNode->match();
          } else {
            rdm = roadmap::fail;
          }
          break;
        }

        case roadmap::producted: {
          // dbg( "case producted" );
          rdm = roadmap::matching;
          break;
        }

        case roadmap::rangeunmatched:
          [[fallthrough]];
          // dbg( "case rangeunmatched" );
        case roadmap::rangematched: {
          // dbg( "case rangematched" );
          auto pNode = pStack->get_last_work();
          assert( pNode );
          // dbg( pNode->name );
          rdm = pNode->check_range( rdm );
          if ( roadmap::matching == rdm ) {
            pNode->push_result_scope_end();
          }
          break;
        }

        case roadmap::bypassmatched:
          [[fallthrough]];
          // dbg( "case bypassmatched" );
        case roadmap::matched: {
          // dbg( "case matched" );
          auto pNode = pStack->get_last_work();
          assert( pNode );
          // dbg( pNode->name );
          if ( rdm == roadmap::matched && !pNode->compound() ) {
            // dbg( pNode->name );
            last_match.used = true;
            last_match.node_name = pNode->name;
            last_match.match_bi = pNode->match_bi;
            last_unmatch.used = false;
          }
          pNode->push_result_scope_end();
          pNode->report_status( rdm );
          if ( pNode->compound() ) {
            ++compound_matched_count;
          }
          rdm = roadmap::collcapse;
          break;
        }

        case roadmap::unmatched: {
          // dbg( "case unmatched" );
          auto pNode = pStack->get_last_work();
          assert( pNode );
          // dbg( pNode->name );
          if ( !last_unmatch.used ) {
            last_unmatch.used = true;
            last_unmatch.node_name = pNode->name;
            last_unmatch.match_bi = pNode->match_bi;
          }
          pNode->report_status( rdm );
          pNode->unwind_result();
          pNode->pop_range_state();
          if ( pNode->compound() ) {
            ++compound_unmatch_count;
          }
          rdm = roadmap::collcapse;
          break;
        }

        case roadmap::collcapse: {
          // dbg( "case collapse" );
          auto packed_count = pStack->collapse();
          // dbg( "collcapse-count: ", packed_count, "remain:", pStack->node_list.size() );
          rdm = roadmap::matching;
          if ( pStack->empty() ) {
            rdm = ( pSource->eof() ) ? roadmap::success : roadmap::fail;
          }
          // dbg( pStack->empty(), pSource->eof() );
          break;
        }

        // -Wswitch
        case roadmap::init:;
        case roadmap::fail:;
        case roadmap::success:;

      } // switch
        // auto ch = _getwch();
        // if ( ch == 'q' ) rdm = roadmap::fail;
    }
    assert( loop_count < MAX_LOOP_COUNT );
    return rdm;
  } // parse

public:
  Source<E> *pSource = nullptr;
  Stack<E> *pStack = nullptr;
  Result *pResult = nullptr;
  Pool<Range> *pRangepool;

public:
  roadmap rdm = roadmap::fail;
  int64_t loop_count;
  MatchCursor last_match;
  MatchCursor last_unmatch;

public:
  int64_t compound_matched_count = 0;
  int64_t compound_unmatch_count = 0;

public:
  RSWList<E> rslist;
}; // Parser

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
template <typename C> struct reverse_wrapper {
  C &c_;
  reverse_wrapper( C &c ) : c_( c ) {}

  typename C::reverse_iterator begin() { return c_.rbegin(); }
  typename C::reverse_iterator end() { return c_.rend(); }
};

template <typename C, size_t N> struct reverse_wrapper<C[N]> {
  C ( &c_ )[N];
  reverse_wrapper( C ( &c )[N] ) : c_( c ) {}

  typename std::reverse_iterator<const C *> begin() { return std::rbegin( c_ ); }
  typename std::reverse_iterator<const C *> end() { return std::rend( c_ ); }
};

template <typename C> reverse_wrapper<C> reverse( C &c ) { return reverse_wrapper<C>( c ); }

//----------------------------------------------------------------------------------------------
}; // namespace bnf

#endif
