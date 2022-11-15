#include <time.h>
#include <process.h>
#include <machine.h>
#include <utility/fork.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "=====Test Task Fork "<< Task::self()->id()<<"=====" << endl;

    if (Task::self()->id() == 0) {
        fork(&main);
        fork(&main);
        cout << "Hello World! I'm Task: "<< Task::self()->id() << endl;
    }
    if (Task::self()->id() == 1) {
        cout << "I'm other task: "<< Task::self()->id() << endl;
    }
    if (Task::self()->id() == 2) {
        cout << "I'm other, other task: "<< Task::self()->id() << endl;
    }

    return 0;
}