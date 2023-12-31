// { dg-do assemble { xfail *-*-* } }
// Templates defined outside must be declared inside
namespace bar
{
  template<class T>
  void foo(); // trick it to provide some prior declaration
  template<class T>class X; // { dg-error "" } previous declaration
}

template <typename T>
T const
bar::foo(T const &a)    
{                        // { dg-error "" "" { xfail *-*-* } } not declared in bar - 
  return a;
}

template<> void bar::foo<int>()
{                        // { dg-error "" "" { xfail *-*-* } } not declared in bar - 
}

template<class T,class U>
class bar::X{};         // { dg-error "" } does not match declaration
