#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <iostream>

#define COMP_ID_START ((cur >= 65 && cur <= 90) || (cur >= 97 && cur <= 122) ||  cur == '_')
#define COMP_NUM (cur >= 48 && cur <= 57)
using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input)
    :in_(input)
{
    NextToken();
    // Реализуйте конструктор самостоятельно
}

const Token& Lexer::CurrentToken() const {
    if(token_.has_value()) {
        return token_.value();
    }
    throw std::logic_error("Not implemented"s);
}

Token Lexer::NextToken() {
    if(auto result = Read(); result.has_value()) {
        token_ = std::move(result);
        return token_.value();
    }
    throw std::logic_error("Not implemented"s);
}

std::optional<Token> Lexer::Read() {
    string result;
    char cur;
    in_.get(cur);
    return Read(cur);
}

std::optional<Token> Lexer::Read(char cur)
{
    if (auto result = CheckAtIndent(cur); result.has_value()) {
        return result.value();
    }
    SkipSpace(cur);
    SkipAtComment(cur);
    if(auto result = CheckAtSpecChar(cur); result.has_value()) {
        return result.value();
    }
    if(auto result = CheckAtChar(cur); result.has_value()) {
        return result.value();
    } else if(auto word = ReadWord(cur); word.has_value()) {
        if(auto spec_word = CheckAtSpecWord(word.value()); spec_word.has_value()) {
            return spec_word.value();
        }
        return Token(token_type::Id{std::move(word.value())});
    } else if (auto result = ReadNumber(cur); result.has_value()) {
        //const char* tmp = result.value().c_str();
        return Token(token_type::Number{atoi(result.value().c_str())});
    } else if (auto result = ReadString(cur); result.has_value()) {
        return Token(token_type::String{std::move(result.value())});
    }
    return nullopt;
}

std::optional<string> Lexer::ReadWord(char cur) {
    if(COMP_ID_START ) {
        string result{cur};
        in_.get(cur);
        while(in_.good() && (COMP_ID_START || COMP_NUM) ) {
            result += cur;
            in_.get(cur);
        }
        in_.putback(cur);
        return result;
    }
    return std::nullopt;
}

std::optional<string> Lexer::ReadNumber(char cur) {
    if(COMP_NUM) {
        string result;
        while(in_.good() && COMP_NUM) {
            result += cur;
            in_.get(cur);
        }
        in_.putback(cur);
        return result;
    }
    return std::nullopt;
}

std::optional<string> Lexer::ReadString(char cur) {
    string result;
    if((cur == '\"') || (cur == '\'')) {
        char begin = cur;
        in_.get(cur);
        while((cur != begin)) {
            if( cur == '\\' ) {
                in_.get(cur);
                if(cur == '"') {
                    result += '"';
                } else if(cur == '\'') {
                    result += '\'';
                } else if(cur == 't') {
                    result += '\t';
                } else if(cur == 'n') {
                    result += '\n';
                }
                in_.get(cur);
                continue;
            }
            result += cur;
            in_.get(cur);
        }
        return result;
    }
    return std::nullopt;
}

std::optional<Token> Lexer::CheckAtSpecWord(string_view word)
{
    static std::unordered_map<std::string_view, Token> tokens;
    if(tokens.empty()) {
        tokens.emplace("class"sv, token_type::Class{});
        tokens.emplace("return"sv, token_type::Return{});
        tokens.emplace("if"sv, token_type::If{});
        tokens.emplace("else"sv, token_type::Else{});
        tokens.emplace("def"sv, token_type::Def{});
        tokens.emplace("print"sv, token_type::Print{});
        tokens.emplace("or"sv, token_type::Or{});
        tokens.emplace("None"sv, token_type::None{});
        tokens.emplace("and"sv, token_type::And{});
        tokens.emplace("not"sv, token_type::Not{});
        tokens.emplace("True"sv, token_type::True{});
        tokens.emplace("False"sv, token_type::False{});
    }
    if (tokens.count(word) != 0) {
        return tokens.at(word);
    }
    return nullopt;
}
//    if(word == "class"sv) {
//        return Token(token_type::Class{});
//    } else if (word == "return"sv) {
//        return Token(token_type::Return{});
//    } else if (word == "if"sv) {
//        return Token(token_type::If{});
//    } else if (word == "else"sv) {
//        return Token(token_type::Else{});
//    } else if (word == "def"sv) {
//        return Token(token_type::Def{});
//    } else if (word == "print"sv) {
//        return Token(token_type::Print{});
//    } else if (word == "or"sv) {
//        return Token(token_type::Or{});
//    } else if (word == "None"sv) {
//        return Token(token_type::None{});
//    } else if (word == "and"sv) {
//        return Token(token_type::And{});
//    } else if (word == "not"sv) {
//        return Token(token_type::Not{});
//    } else if (word == "True"sv) {
//        return Token(token_type::True{});
//    } else if (word == "False"sv) {
//        return Token(token_type::False{});
//    }
//    return nullopt;


std::optional<Token> Lexer::CheckAtIndent(char cur)
{
    // если идет два переноса строки подряд
    if(token_.has_value() && (token_.value().Is<token_type::Newline>()) && cur == '\n') {
       // пропускаем идущие подряд перносы строк
       while(cur == '\n' && !in_.eof()) {
           in_.get(cur);
       }
       if(!in_.eof()) {
          in_.putback(cur);
          cur = '\n';
       }
    }
    // если начало программы
    if(!count_space_.has_value()) {
        count_space_ = cur == ' ' ? 1 : 0;
        if(cur == ' ') {
            while (cur == ' ') {
                ++count_space_.value();
                in_.get(cur);
            }
            in_.putback(cur);
        }
    }
    if(token_.has_value()
       && (token_.value().Is<token_type::Newline>() || count_consider_space_ > 0)
       && count_space_.has_value()
       && (cur != '\n' || in_.eof()))
    {
        int count_space = 0;
        if(cur == ' ') {
            while (cur == ' ') {
                ++count_space;
                in_.get(cur);
            }
            //in_.putback(cur);
        }
        if(count_consider_space_ > 0) {
            count_consider_space_ -= 2;
            count_space_.value() -= 2;// моделирует сдвиг по одному отступу за проход
            in_.putback(cur); // если остался не обработтанный дедента и начало строки
            return Token(token_type::Dedent{});
        } else if((count_space - count_space_.value()) == 2) {
            count_space_ = count_space;
            in_.putback(cur);
            return Token(token_type::Indent{});
        } else if (count_space_.value() - count_space == 2) {
            count_space_ = count_space;
            in_.putback(cur);
            return Token(token_type::Dedent{});
        } else if (((count_space_.value() - count_space > 0))) {
            count_consider_space_ = count_space_.value() - count_space;
            count_consider_space_ -= 2;
            count_space_.value() -= 2;// моделирует сдвиг по одному отступу за проход
            in_.putback(cur); // если два дедента подряд и начало строки
            return Token(token_type::Dedent{});
        } else {
            if(count_space != 0) {
                in_.putback(cur); // если пробелы были, но сдвига относительно прошлой строки не было
            }
        }

    }
    return nullopt;
}

std::optional<Token> Lexer::CheckAtChar(char cur) {
    if(cur == '+') {
        return Token(token_type::Char{'+'});
    } else if (cur == '-') {
        return Token(token_type::Char{'-'});
    } else if (cur == '*') {
        return Token(token_type::Char{'*'});
    } else if (cur == '/') {
        return Token(token_type::Char{'/'});
    } else if (cur == '(') {
        return Token(token_type::Char{'('});
    } else if (cur == ')') {
        return Token(token_type::Char{')'});
    } else if (cur == '.') {
        return Token(token_type::Char{'.'});
    } else if (cur == ',') {
        return Token(token_type::Char{','});
    } else if (cur == ':') {
        return Token(token_type::Char{':'});
    } else if (cur == '=') {
        in_.get(cur);
        if(cur == '=' && !in_.eof()) {
            return Token(token_type::Eq{});
        }
        in_.putback(cur);
        return Token(token_type::Char{'='});
    }
    else if (cur == '>') {
        in_.get(cur);
        if(cur == '=') {
            return Token(token_type::GreaterOrEq{});
        }
        in_.putback(cur);
        return Token(token_type::Char{'>'});
    }
    else if (cur == '<') {
        in_.get(cur);
        if(cur == '=') {
            return Token(token_type::LessOrEq{});
        }
        in_.putback(cur);
        return Token(token_type::Char{'<'});
    }
    else if (cur == '!') {
        in_.get(cur);
        if(cur == '=') {
            return Token(token_type::NotEq{});
        }
        in_.putback(cur);
    }
    return nullopt;
}

void Lexer::SkipAtComment(char& cur)
{
    if(cur == '#') {
        string tmp;
//        in_.ignore(numeric_limits<streamsize>::max(), '\n');
        while (cur != '\n' &&  ! in_.eof()) {
            in_.get(cur);
        }
    }
}

std::optional<Token> Lexer::CheckAtSpecChar(char& cur)
{
    if (cur == '\n') {
        if(token_.has_value() && ! token_.value().Is<token_type::Newline>()) {
            return Token(token_type::Newline{});
        }
        //in_.get(cur);
        while(cur == '\n' && !in_.eof()) {
            in_.get(cur);
        }
        if(in_.eof()) {
            return Token(token_type::Eof{});
        }
        return Read(cur);
        // && token_.value().Is<token_type::Newline>()
    }
    if (cur == '\0' || !in_.good()) {
        if(token_.has_value() && ! token_.value().Is<token_type::Newline>() && ! token_.value().Is<token_type::Eof>()
           && ! token_.value().Is<token_type::Dedent>()) {
            return Token(token_type::Newline{});
        }
        return Token(token_type::Eof{});
    }
    if(in_.eof()) {
        return Token(token_type::Eof{});
    }
    return nullopt;
}

void Lexer::SkipSpace(char& cur) {
    if(cur == ' ') {
        in_ >> cur;
    }
}

}  // namespace parse
