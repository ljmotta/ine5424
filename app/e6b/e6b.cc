#include <utility/ostream.h>

using namespace EPOS;

OStream cout;

const int arr_size = 10;
const int children_size = 1000;

int main()
{
    cout << "Hello e6b!" << endl;

    int** arr = new int*[arr_size]; // pointer array
    cout << "   arr, " << arr << endl; // print the address of the array
    arr[0] = new int[children_size + 100]; // bigger space
    cout << "arr[0], " << arr[0] << endl;
    arr[1] = new int[children_size - 100];
    cout << "arr[1], " << arr[1] << endl;
    delete arr[0]; // free

    for (int i = 2; i < arr_size; i++) {
        arr[i] = new int[children_size]; // alloc in each interation
        cout << "arr[" << i << "], " << arr[i] << endl; // print the address of each element
    }

    return 0;
}
