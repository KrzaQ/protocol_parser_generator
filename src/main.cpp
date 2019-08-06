#include <iostream>

#define DEBUG(x) std::cout << __FILE__ << ":" << __LINE__ << ":" <<\
    #x << ":" << x << std::endl;

#include "protocol_parser_generator/proto.hpp"

namespace kq
{
using mov = message<
    element<struct begin, char_constant<'['>>,
    element<struct message_type, text<8>>,
    element<struct x_to, number<3>>,
    element<struct y_to, number<3>>,
    element<struct end, char_constant<']'>>
>;

}


int main()
{
    kq::mov m;
    m.value<kq::message_type>() = "MOV";
    m.value<kq::x_to>() = 13;
    m.value<kq::y_to>() = 37;
    DEBUG(m.to_string());

    kq::mov m2 = kq::mov::parse(m.to_string);

    DEBUG(m2.value<kq::message_type>());
    DEBUG(m2.value<kq::x_to>());
    DEBUG(m2.value<kq::y_to>());
}
