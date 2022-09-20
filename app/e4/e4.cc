#include <utility/ostream.h>
#include <process.h>

using namespace EPOS;

OStream cout;

const unsigned int thread_num = 5;
Thread * t[thread_num];
int code;

int func_a() {
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
  Thread * c = new Thread(Thread::Configuration(Thread::READY, Thread::NORMAL-1), &func_b);
  c->join();
  cout << "\nThe end!" << endl;
  delete c;
  return 0;
}