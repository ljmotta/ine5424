#include <process.h>
#include <utility/ostream.h>
#include <time.h>

using namespace EPOS;
OStream cout;

#define WAIT_TIME 50000

const unsigned int thread_num = 5;
Thread * t[thread_num];
int code;

int func_a() {
  Delay wait_micro_sec(WAIT_TIME);
  cout << code-- << " " << endl;
  return 0;
}

int func_b() {
  for (unsigned int i = 0; i < thread_num; ++i) {
    t[i] = new Thread(&func_a);
    cout << code++ << " " << endl;
  }
  for (unsigned int i = 0; i < thread_num; ++i) {
    t[i]->join();
    delete t[i];
  }
  return 0;
}

int main()
{
  cout << "Simple Test" << endl;
  Thread * c = new Thread(&func_b);
  c->join();
  delete c;
  cout << "\nThe end!" << endl;
  return 0;
}