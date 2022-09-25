#include <utility/ostream.h>
#include <machine/otp.h>

using namespace EPOS;

OStream cout;
SiFive_OTP otp;
const int BUF_SIZE = sizeof(int) * 8;


int main()
{
    int buf[BUF_SIZE];
    int a = otp.read(0xfe, &buf, BUF_SIZE);
    cout << "Hello world!" << a << endl;

    return 0;
}