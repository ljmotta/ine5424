#include <utility/ostream.h>

using namespace EPOS;

OStream cout;

const int arr_size = 5;

bool is_sorted(int** arr, int s) {
    for (int i = 0; i < (s - i); i++) {
        if (arr[i] < arr[i + 1]) {
            return false;
        }
    }
    return true;
}

int main()
{
    cout << "Hello e6b!" << endl;

    int** arr = new int*[arr_size]; // pointer array
    for (int i = 0; i < arr_size; i++) {
        arr[i] = new int[1000]; // alloc in each interation
    }

    for (int i = 0; i < arr_size; i++) {
        cout << arr[i] << endl; // print the address of each element
    }

    cout << endl << is_sorted(arr, arr_size) << endl << endl;

    return 0;
}
