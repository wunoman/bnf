<img src="https://www.freeiconspng.com/uploads/c--logo-icon-0.png">

# The BNF(EBNF) Parser
version 0.0.0.1 (using C++17)

## Lexical
parse input ( string or file ) to tokens

## Syntax
using lexcial parse output as input, setup first and follow set and parse tokens according to
BNF(EBNF) rules.

## Note
since without left recursion check, eliminate left recursion rule before you parse it, see more detail in lua parse examle in llua.cpp

## Demo
+ seque, alter and terminal ( implemented by functions )

seque as: T==> A B C
```c++
bnf::Nseque<uchar> S{ "S" };
S( A, B, C );
```
alter as: T==> A|B|C
```c++
bnf::Nalter<uchar> S{ "S" };
S( A, B, C );
```
string as: T==> "something"
```c++
bnf::Nliteral<uchar> T{ "name of something", "something", nid_something };
```
state machine funcions as: T==> 'a'|'b'|...'z'
```c++
bnf::Nfunc<uchar> f{ "lowletter", _f, _frs, nid_something };
_f = [ Nfunc<uchar> &node ]->roadmap {
  ...
  return (ch>='a' && ch<='z')?matched:unmatched;
}
_frs = [ char ch ]->roadmap {
  return (ch>='a' && ch<='z')?matched:unmatched;
}
```
with range as: T==> 'a'{1,4}, min value is zero means allow match nothing, when max equal -1 match any number input
```c++
bnf::Nliteral<uchar> T{ "name", "a", nid_a, 1, 4 };
```

+ lua syntax parser ( match hexical number not implemented yet! )
in file llua.h and llua.cpp

## Output
output text format is Lua string, include success or fail, stack and result size used, matched or unmatched count,
if fail should print the information of match cursor

## Support
wunoman@qq.com

## License
Licensed under the [MIT License](https://www.lua.org/license.html).

