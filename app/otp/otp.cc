#include <utility/ostream.h>
#include <machine/otp.h>

using namespace EPOS;

OStream cout;
SiFive_OTP otp;
const int BUF_SIZE = sizeof(int) * 8;


int main()
{
    unsigned int buf[BUF_SIZE];
    int a = otp.read(0x00, &buf, BUF_SIZE);
    cout << "size=" << a << endl;
    cout << "buf=" << buf[0] << endl;
    cout << "buf=" << buf[1] << endl;
    cout << "buf=" << buf[2] << endl;
    cout << "buf=" << buf[3] << endl;
    cout << "buf=" << buf[4] << endl;
    cout << "buf=" << buf[5] << endl;
    cout << "buf=" << buf[6] << endl;
    cout << "buf=" << buf[7] << endl;

    return 0;
}