#include <utility/ostream.h>
#include <machine/otp.h>

using namespace EPOS;

OStream cout;
SiFive_OTP otp;
const int BUF_SIZE = 3840; // /4 -> 
const int offset = 0xfc; //0xf8 -> 252 foi para 190

int main()
{
    unsigned int write_buffer[BUF_SIZE];
    for(int i = 0; i < BUF_SIZE; i++) {
        write_buffer[i] = 0;
    }
    write_buffer[BUF_SIZE - 2] = 0xf;
    write_buffer[BUF_SIZE - 1] = 0x2;

    int b = otp.write(offset *4, &write_buffer, (BUF_SIZE - offset) * 4);
    cout << "write size=" << b << endl;
    cout << hex << ", buf=" << write_buffer[BUF_SIZE -1] << endl;
    cout << hex << ", buf=" << write_buffer[BUF_SIZE -2] << endl;

    unsigned int read_buffer[BUF_SIZE]; // buffer de 4byts * buf_size
    int c = otp.read(offset *4, &read_buffer, (BUF_SIZE - offset) *4);
    cout << "read size=" << c << endl;
    cout << hex << ", buf=" << read_buffer[0] << endl;
    cout << hex << ", buf=" << read_buffer[1] << endl;

    // for(int j = 0; j < BUF_SIZE; j++) {
        // cout << ", index= " << hex << offset + j << ", buf=" << read_buffer[j] << endl;
    // }
    cout << hex << ", buf=" << read_buffer[BUF_SIZE -1] << endl;
    cout << hex << ", buf=" << read_buffer[BUF_SIZE -2] << endl;


    return 0;
}