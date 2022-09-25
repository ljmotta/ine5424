#include <utility/ostream.h>
#include <machine/otp.h>

using namespace EPOS;

OStream cout;

int main()
{
    void *buf;
    int size = sizeof(int) * 8;
    int a = SiFive_OTP::read(0xfe, buf, size);
    cout << "Hello world!" << a << endl;

    return 0;
}