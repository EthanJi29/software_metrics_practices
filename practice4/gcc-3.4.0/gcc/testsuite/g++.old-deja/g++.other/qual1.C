// { dg-do assemble  }
// { dg-options "-O" }
// Origin: Benjamin Pflugmann <philemon@spin.de>

// DR 295 allows qualification via typedef

typedef const char *(func_type)();

class
{
public:
  func_type *Function;
  // The following is DR 295 dependent
  const func_type* function(void) { return Function; } // { dg-error "" } constifying
  volatile func_type* functionv(void); // { dg-error "" } qualifier
} action;

void work(const char *source)
{
  work( action.function()() );
}
