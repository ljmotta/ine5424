#include <utility/ostream.h>
#include <process.h>

using namespace EPOS;

OStream cout;

void * p1 = new (SHARED) void *;
void * p2 = new (SHARED) void *;

const char ** _before = new (SHARED) const char *;
const char ** _after = new (SHARED) const char *;

int before() {
    * _before = "This Thread Runs Before";
    return 0;
}

int after() {
    * _after = "This Thread Runs After";
    return 0;
}

 int main()
 {
  cout << "Start Shared Memory Allocator Test" << endl;
  assert(p1 == p2);

  char ** c1 = reinterpret_cast<char **>(p1);
  char ** c2 = reinterpret_cast<char **>(p2);

  char buff[32] = "This memory is being shared\0";
  *c1 = buff;

  cout << *c2 << endl;

  Thread * before_t = new Thread(&before);
  before_t->join();
  cout << *_after << endl;

  Thread * after_t = new Thread(&after);
  after_t->join();
   cout << *_before << endl;
 return 0;
 }