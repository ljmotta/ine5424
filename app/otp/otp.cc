#include <utility/ostream.h>
#include <machine/otp.h>

using namespace EPOS;

OStream cout;
OTP otp;

const int WRITE_OFFSET = 0;
const int WRITE_OFFSET_BITS = WRITE_OFFSET * 4;
const int WRITE_SIZE_BITS = 0;
const int WRITE_SIZE_BUFFER = WRITE_SIZE_BITS / 4;

const int READ_OFFSET = 0xfc; // serial position
const int READ_OFFSET_BITS = READ_OFFSET * 4;
const int READ_SIZE_BITS = 15360; // all useful memory
const int READ_SIZE_BUFFER = READ_SIZE_BITS / 4;

int main()
{
    // create write buffer
    unsigned int write_buffer[WRITE_SIZE_BUFFER];
    for(int i = 0; i < WRITE_SIZE_BUFFER; i++) {
        write_buffer[i] = 0x4; // aribtrary data
    }

    // write in otp memory
    int write_code = otp.write(WRITE_OFFSET_BITS, &write_buffer, WRITE_SIZE_BITS);
    if (write_code < 0) {
        cout << "write error, code = " << write_code << endl;
    } else {
        cout << hex << "write, buf=" << write_buffer[0] << endl;
        cout << hex << "write, buf=" << write_buffer[1] << endl;
    }

    // create read buffer
    unsigned int read_buffer[READ_SIZE_BUFFER];

    // read otp memory
    int read_code = otp.read(READ_OFFSET_BITS, &read_buffer, READ_SIZE_BITS);
    if (read_code < 0) {
        cout << "read error, code = " << read_code << endl;
    } else {
        for(int j = 0; j < READ_SIZE_BUFFER; j++) {
            cout << "read, index= " << hex << READ_OFFSET + j << ", buf=" << read_buffer[j] << endl;
        }
    }

    return 0;
}