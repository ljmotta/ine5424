#include <utility/ostream.h>
#include <machine/otp.h>

using namespace EPOS;

OStream cout;
OTP otp;

const int WRITE_FUSES_OFFSET = 0xfe;
const int WRITE_OFFSET_BYTES = WRITE_FUSES_OFFSET * 4;
const int WRITE_SIZE_BYTES = 12; // qtt of bytes
const int WRITE_FUSES_BUFFER = WRITE_SIZE_BYTES / 4; // qtt of fuses

const int READ_FUSES_OFFSET = 0xfc; // serial position
const int READ_OFFSET_BYTES = READ_FUSES_OFFSET * 4;
const int READ_SIZE_BYTES = 28; // bytes
const int READ_FUSES_BUFFER = READ_SIZE_BYTES / 4;

int main()
{
    // create write buffer
    unsigned int write_buffer[WRITE_FUSES_BUFFER];
    for(int i = 0; i < WRITE_FUSES_BUFFER; i++) {
        write_buffer[i] = 0x42; // arbitrary data
    }

    // write in otp memory
    int write_code = otp.write(WRITE_OFFSET_BYTES, &write_buffer, WRITE_SIZE_BYTES);
    if (write_code < 0) {
        cout << "write error, code = " << write_code << endl;
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

    cout << "\n\n write again with higher offset (+4) \n\n" << endl;

    // try to write in the same place
    for(int i = 0; i < WRITE_FUSES_BUFFER; i++) {
        write_buffer[i] = 0x33; // arbitrary data
    }

    // write in otp memory
    int write_again_code = otp.write(WRITE_OFFSET_BYTES + 4, &write_buffer, WRITE_SIZE_BYTES);
    if (write_again_code < 0) {
        cout << "write again error, code = " << write_again_code << endl;
    }

    // clean read buffer
    for(int i = 0; i < READ_FUSES_BUFFER; i++) {
        read_buffer[i] = 0x00; // arbitrary data
    }
    // read otp memory
    int read_again_code = otp.read(READ_OFFSET_BYTES, &read_buffer, READ_SIZE_BYTES);
    if (read_again_code < 0) {
        cout << "read again error, code = " << read_again_code << endl;
    } else {
        for(int j = 0; j < READ_FUSES_BUFFER; j++) {
            cout << "read again, index= " << hex << READ_FUSES_OFFSET + j << ", buf=" << read_buffer[j] << endl;
        }
    }

    return 0;
}