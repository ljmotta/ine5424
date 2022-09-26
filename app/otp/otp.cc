#include <utility/ostream.h>
#include <machine/otp.h>

using namespace EPOS;

OStream cout;
SiFive_OTP otp;

const int SIZE = 15360; // in bits
const int OFFSET = 0xfc; // in bytes
const int OFFSET_BITS = OFFSET * 4; // in bits
const int BUF_SIZE = SIZE / 4; // int buffer, 4byts

int main()
{
    unsigned int write_buffer[BUF_SIZE];
    for(int i = 0; i < BUF_SIZE; i++) {
        write_buffer[i] = 0x0;// aribtrary data
    }

    int b = otp.write(OFFSET_BITS, &write_buffer, SIZE);
    cout << "write size=" << b << endl;
    cout << hex << "write, buf=" << write_buffer[0] << endl;
    cout << hex << "write, buf=" << write_buffer[1] << endl;

    unsigned int read_buffer[BUF_SIZE];
    int c = otp.read(OFFSET_BITS, &read_buffer, SIZE);
    cout << "read size=" << c << endl;

    for(int j = 0; j < BUF_SIZE; j++) {
        cout << ", index= " << hex << OFFSET + j << ", buf=" << read_buffer[j] << endl;
    }

    return 0;
}