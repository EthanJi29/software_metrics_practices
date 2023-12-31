// { dg-do assemble  }
// { dg-options "-Wreturn-type" }
// GROUPS passed error-reporting

// DR 295 allows qualification via typedef

template <char C>
class badoo
{
};

template <int (*F) (int)>
class doowop
{
};

struct A
{
  int a;
  ~A () { a = 0; }
  operator int () { return a; }
};

extern "C" int atoi (char *);

int (*fee)(char *) = atoi;
int (**bar)(char *) = &fee;

char *s = "4";
char **sp = &s;
char ***spp = &sp;

int foo (int a = (**bar) (s))
{
   return doowop<foo>::bar; // { dg-error "" } not a member
}

int foo2 (int (*a)(int) = &foo)
{
   undef4 (1); // { dg-error "" } implicit declaration
  return 1;
}

class X{
  class Y{};			// { dg-error "" } private
};

typedef int const * bart ();
//The following is DR295 dependant
typedef bart const * const * bar2; // { dg-error "" } constifying qualifiers
typedef bart volatile * const * bar2v; // { dg-error "" } qualifiers

bar2 baz (X::Y y)
{				// { dg-error "" } in this context
  X::Y f;			// { dg-error "" } in this context
  bar2 wa [5];
  wa[0] = baz(f);
  undef2 (1); // { dg-error "" } implicit declaration
}

int ninny ()
{
  struct A
    {
	  static int ninny2 () { return badoo<'\001'>::foo; } // { dg-error "" } not a member
    };

  return A::ninny2();
}

int darg (char X::*p)
{
   undef3 (1); // { dg-error "" } implicit declaration
}
