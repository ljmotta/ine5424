#include<time.h>
#include <synchronizer.h>
#include <process.h>

using namespace EPOS;
const int iterations = 10;
OStream cout; 
Semaphore lets_call_idle(0); 

void release_lock() {
   lets_call_idle.v();
} 

int main() {
   cout << "Simple Idle Test" << endl;
   Function_Handler fh(&release_lock);
   Alarm a(1000000,&fh, iterations);
   for (int i = 0; i < iterations; i++) {
       lets_call_idle.p();
       cout << "Main was Idle " << i+1 << " times" << endl;
   }

   cout << "The end!" << endl;
   return 0;
}