#include <utility/ostream.h>
#include <machine/otp.h>

using namespace EPOS;

OStream cout;
SiFive_OTP otp;
const int BUF_SIZE = 1024;
const int offset = 0x00;

int main()
{
    // unsigned int buf[BUF_SIZE];
    // int a = otp.read(0x00, &buf, BUF_SIZE);
    // cout << "read size=" << a << endl;

    // for(int i = 0; i < BUF_SIZE; i++) {
    //     cout << "index= " << i << ", buf=" << buf[i] << endl;
    // }

    // unsigned int write_buffer[BUF_SIZE];
    // for(int i = 0; i < BUF_SIZE; i++) {
    //     write_buffer[i] = 0;
    // }
    // write_buffer[1] = 212;
    // write_buffer[7] = 2;
    // int b = otp.write(0x00, &write_buffer, BUF_SIZE);
    // cout << "write size=" << b << endl;

    unsigned int read_buffer[BUF_SIZE];
    int c = otp.read(offset, &read_buffer, BUF_SIZE);
    cout << "read size=" << c << endl;

    for(int j = 0; j < BUF_SIZE; j++) {
        cout << "index= " << j << ", buf=" << read_buffer[j] << endl;
    }

    return 0;
}