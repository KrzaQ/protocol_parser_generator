#include <iostream>

#define DEBUG(x) std::cout << __FILE__ << ":" << __LINE__ << ":" <<\
    #x << ":" << x << std::endl;

#include "proto.hpp"

namespace kq
{
using mcr = message<
    element<struct begin, char_constant<'['>>,
    element<struct message_type, text<8>>,
    element<struct x_to, number<3>>,
    element<struct y_to, number<3>>,
    element<struct end, char_constant<']'>>
>;

}


int main()
{
    // DEBUG(f2.value<name>());
    // DEBUG(f2.value<age>());
}
