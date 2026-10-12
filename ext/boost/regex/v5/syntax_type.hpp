
#ifndef BOOST_REGEX_SYNTAX_TYPE_HPP
#define BOOST_REGEX_SYNTAX_TYPE_HPP

namespace boost{
namespace regex_constants{

typedef unsigned char syntax_type;

static const syntax_type syntax_char = 0;
static const syntax_type syntax_open_mark = 1;
static const syntax_type syntax_close_mark = 2;
static const syntax_type syntax_dollar = 3;
static const syntax_type syntax_caret = 4;
static const syntax_type syntax_dot = 5;
static const syntax_type syntax_star = 6;
static const syntax_type syntax_plus = 7;
static const syntax_type syntax_question = 8;
static const syntax_type syntax_open_set = 9;
static const syntax_type syntax_close_set = 10;
static const syntax_type syntax_or = 11;
static const syntax_type syntax_escape = 12;
static const syntax_type syntax_dash = 14;
static const syntax_type syntax_open_brace = 15;
static const syntax_type syntax_close_brace = 16;
static const syntax_type syntax_digit = 17;
static const syntax_type syntax_comma = 27;
static const syntax_type syntax_equal = 37;
static const syntax_type syntax_colon = 36;
static const syntax_type syntax_not = 53;

static const syntax_type syntax_hash = 13;
static const syntax_type syntax_newline = 26;

typedef syntax_type escape_syntax_type;

static const escape_syntax_type escape_type_word_assert = 18;
static const escape_syntax_type escape_type_not_word_assert = 19;
static const escape_syntax_type escape_type_control_f = 29;
static const escape_syntax_type escape_type_control_n = 30;
static const escape_syntax_type escape_type_control_r = 31;
static const escape_syntax_type escape_type_control_t = 32;
static const escape_syntax_type escape_type_control_v = 33;
static const escape_syntax_type escape_type_ascii_control = 35;
static const escape_syntax_type escape_type_hex = 34;
static const escape_syntax_type escape_type_unicode = 0;
static const escape_syntax_type escape_type_identity = 0;
static const escape_syntax_type escape_type_backref = syntax_digit;
static const escape_syntax_type escape_type_decimal = syntax_digit;
static const escape_syntax_type escape_type_class = 22;
static const escape_syntax_type escape_type_not_class = 23;

static const escape_syntax_type escape_type_left_word = 20;
static const escape_syntax_type escape_type_right_word = 21;
static const escape_syntax_type escape_type_start_buffer = 24;
static const escape_syntax_type escape_type_end_buffer = 25;
static const escape_syntax_type escape_type_control_a = 28;
static const escape_syntax_type escape_type_e = 38;
static const escape_syntax_type escape_type_E = 47;
static const escape_syntax_type escape_type_Q = 48;
static const escape_syntax_type escape_type_X = 49;
static const escape_syntax_type escape_type_C = 50;
static const escape_syntax_type escape_type_Z = 51;
static const escape_syntax_type escape_type_G = 52;

static const escape_syntax_type escape_type_property = 54;
static const escape_syntax_type escape_type_not_property = 55;
static const escape_syntax_type escape_type_named_char = 56;
static const escape_syntax_type escape_type_extended_backref = 57;
static const escape_syntax_type escape_type_reset_start_mark = 58;
static const escape_syntax_type escape_type_line_ending = 59;

static const escape_syntax_type syntax_max = 60;

}
}

#endif
