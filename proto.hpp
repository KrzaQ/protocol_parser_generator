#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <exception>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace kq
{

struct buffer_too_small : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct input_too_small : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct invalid_input : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct invalid_data : std::runtime_error {
    using std::runtime_error::runtime_error;
};

template<class Key, class Type>
struct element
{
    using key = Key;
    using type = Type;
};

namespace detail
{

template<typename T> struct element_value_type {
    using type = typename T::type::value_type;
};

template<typename... Elements>
struct type_list {
    static constexpr size_t size = sizeof...(Elements);

    template<size_t N>
    using nth_element = typename std::tuple_element<
        N, std::tuple<Elements...>
    >::type;
};

template<size_t, typename, typename> struct type_to_list_index_impl{};

template<size_t N, typename Sought, typename Current, typename... Others>
struct type_to_list_index_impl<N, Sought, type_list<Current, Others...>>
{
    static constexpr size_t value = std::conditional_t<
        std::is_same_v<typename Current::key, Sought>,
        std::integral_constant<size_t, N>,
        type_to_list_index_impl<N+1, Sought, type_list<Others...>>
    >::value;
};

template<typename Sought, typename TypeList>
static constexpr size_t type_to_list_index =
    type_to_list_index_impl<0, Sought, TypeList>::value;

template<size_t N> struct type_to_hold_number_impl {
    using type = typename type_to_hold_number_impl<N+1>::type;
};

template<> struct type_to_hold_number_impl<2> { using type = int8_t; };
template<> struct type_to_hold_number_impl<4> { using type = int16_t; };
template<> struct type_to_hold_number_impl<9> { using type = int32_t; };
template<> struct type_to_hold_number_impl<18> { using type = int64_t; };

template<size_t N>
using type_to_hold_number = typename type_to_hold_number_impl<N>::type;

template<class, class> struct offset_calculator_impl;

template<typename TypeList, size_t... Is>
struct offset_calculator_impl<TypeList, std::index_sequence<Is...>>
{
    template<size_t N>
    static constexpr size_t offset = (
        (Is < N ? TypeList::template nth_element<Is>::type::length : 0)
        + ...
    );
};

template<typename TypeList>
struct offset_calculator
{
    template<size_t N>
    static constexpr size_t offset = offset_calculator_impl<
        TypeList, std::make_index_sequence<TypeList::size>
    >::template offset<N>;
};

}

template<typename... Elements>
struct message
{
    using members = detail::type_list<Elements...>;

    static constexpr size_t length = (Elements::type::length + ...);

    static constexpr message parse(std::string_view buf) {
        if(buf.size() < length)
            throw input_too_small{":("};

        message m;
        m.parse_impl(buf, std::make_index_sequence<members::size>{});
        return m;
    }

    constexpr void write(char* buf, size_t size) const {
        if(size < length)
            throw buffer_too_small{":("};

        write_impl(buf, std::make_index_sequence<members::size>{});
    }

    std::string to_string() const {
        std::string buf(length, '\0');
        write(buf.data(), buf.size());
        return buf;
    }

    template<typename Key>
    constexpr auto& value() {
        return std::get<detail::type_to_list_index<Key, members>>(data);
    }

    template<typename Key>
    constexpr auto const& value() const {
        return std::get<detail::type_to_list_index<Key, members>>(
            const_cast<message*>(this)->data);
    }

private:

    template<size_t... Is>
    void write_impl(char* buf, std::index_sequence<Is...>) const {
        using offsets = detail::offset_calculator<members>;
        (
            (void)members::template nth_element<Is>::type::write(
                std::get<Is>(data),
                buf + offsets::template offset<Is>
            ), ...
        );
    }

    template<size_t... Is>
    void parse_impl(std::string_view buf, std::index_sequence<Is...>) {
        using offsets = detail::offset_calculator<members>;
        (
            (void)(std::get<Is>(data) =
                members::template nth_element<Is>::type::parse(
                std::string_view{
                    buf.data() + offsets::template offset<Is>,
                    members::template nth_element<Is>::type::length
                }
            )), ...
        );
    }

    std::tuple<typename detail::element_value_type<Elements>::type...> data;
};

template<size_t Length>
struct text
{
    static constexpr size_t length = Length;
    using value_type = std::string;

    static constexpr void write(value_type const& val, char* buf) {
        if(val.size() > length) {
            std::copy_n(val.cbegin(), length, buf);
            return;
        }

        auto it = std::fill_n(buf, length - val.size(), ' ');

        std::copy(val.cbegin(), val.cend(), it);
    }

    static value_type parse(std::string_view buf) {
        while(buf.starts_with(' '))
            buf.remove_prefix(1);

        return value_type{buf};
    }

};

template<size_t Length>
struct number
{
    static constexpr size_t length = Length;
    using value_type = detail::type_to_hold_number<Length>;

    static constexpr void write(value_type const& val, char* buf) {
        std::array<char, length> out_buffer{};
        auto [p, ec] = std::to_chars(
            out_buffer.data(), out_buffer.data() + length, val
        );
        if(ec != std::errc())
            throw invalid_data{""};
        auto it = std::fill_n(buf, length - (p - out_buffer.data()), '0');
        std::copy(out_buffer.data(), p, it);
    }

    static value_type parse(std::string_view buf) {
        value_type ret;
        auto [_, ec] = std::from_chars(buf.data(), buf.data()+length, ret);
        if(ec != std::errc())
            throw invalid_input{std::string{buf}};
        return ret;
    }
};

template<char C>
struct char_constant
{
    static constexpr size_t length = 1;
    using value_type = char; /* for impl simplicity */

    static constexpr value_type value = C;

    static constexpr void write(value_type const&, char* buf) {
        *buf = value;
    }

    static value_type parse(std::string_view buf) {
        if(buf[0] != value)
            throw invalid_input{std::string{buf}};
        return value;
    }
};

}
