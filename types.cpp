#include "types.h"

using namespace std;

ostream& operator<<(ostream& os, EWordOrder val)
{
    switch (val) {
        case EWordOrder::BigEndian:
            return os << "BigEndian";
        case EWordOrder::LittleEndian:
            return os << "LittleEndian";
    }

    return os;
}

uint8_t RegisterFormatSize(ERegisterFormat format)
{
    switch (format) {
        case S64:
        case U64:
        case Double:
            return 8;
        case U32:
        case S32:
        case BCD32:
        case Float:
            return 4;
        case U24:
        case S24:
        case BCD24:
            return 3;
        case Char8:
        case U8:
        case S8:
            return 1;
        default:
            return 2;
    }
}
