#include <utility/ostream.h>

using namespace EPOS;

OStream cout;

int main()
{
    int a = read_otp_word(0xfe);
    cout << "Hello world!" << a << endl;

    return 0;
}