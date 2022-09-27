#include <utility/ostream.h>
#include <machine/otp.h>

using namespace EPOS;

OStream cout;
OTP otp;

const int WRITE_FUSES_OFFSET = 0xfe;
const int WRITE_OFFSET_BYTES = WRITE_FUSES_OFFSET * 4;
const int WRITE_SIZE_BYTES = 64; // qtt of bytes
const int WRITE_FUSES_BUFFER = WRITE_SIZE_BYTES / 4; // qtt of fuses

const int READ_FUSES_OFFSET = 0xfc; // serial position
const int READ_OFFSET_BYTES = READ_FUSES_OFFSET * 4;
const int READ_SIZE_BYTES = 72; // bytes
const int READ_FUSES_BUFFER = READ_SIZE_BYTES / 4;

int main()
{
    // create write buffer
    unsigned int write_buffer[WRITE_FUSES_BUFFER];
    for(int i = 0; i < WRITE_FUSES_BUFFER; i++) {
        write_buffer[i] = 0x12; // arbtrary data
    }

    // write in otp memory
    int write_code = otp.write(WRITE_OFFSET_BYTES, &write_buffer, WRITE_SIZE_BYTES);
    if (write_code < 0) {
        cout << "write error, code = " << write_code << endl;
    } else {
        cout << "write, buf=" << hex << write_buffer[0] << endl;
        cout << "write, buf=" << hex << write_buffer[1] << endl;
    }

    // create read buffer
    unsigned int read_buffer[READ_FUSES_BUFFER];

    // read otp memory
    int read_code = otp.read(READ_OFFSET_BYTES, &read_buffer, READ_SIZE_BYTES);
    if (read_code < 0) {
        cout << "read error, code = " << read_code << endl;
    } else {
        for(int j = 0; j < READ_FUSES_BUFFER; j++) {
            cout << "read, index= " << hex << READ_FUSES_OFFSET + j << ", buf=" << read_buffer[j] << endl;
        }
    }

    return 0;
}