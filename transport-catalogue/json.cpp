#include "json.h"

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

void IgnoreWhiteSpace(istream& input) {
    char c;
    input >> c;
    while(c == ' ' || c == '\n' || c == '\t' || c == '\r') {
        input >> c;
    }
    input.putback(c);
}

Node LoadBool(istream& input) {
    std::string s;
    char c;
    input >> c;
    int size_char = 0;
    if (c == 't') {
        size_char = 4;
    } else if (c == 'f') {
        size_char = 5;
    }
    s += c;
    for (int i = 1; i < size_char; ++i) {
        input >> c;
        s += c;
    }
    IgnoreWhiteSpace(input);
    if (s == "true"s) {
        return Node{true};
    } else if (s == "false"s) {
        return Node{false};
    } else {
        throw ParsingError("ParsingError in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }
}

Node LoadNull(istream& input) {
    std::string s;
    char c;
    input.get(c);
    s += c;
    for (int i = 1; input >> c && i < 4; ++i) {
        s += c;
    }
    IgnoreWhiteSpace(input);
    if (s == "null"s) {
        return Node{nullptr};
    } else {
        throw ParsingError("ParsingError in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }
}

Node LoadArray(istream& input) {
    Array result;
    char c = ' ';
    bool first = true;

    for (; input >> c && c != ']';) {
        first = false;
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (first && c != ']') {
        throw ParsingError("ParsingError in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }

    return Node(move(result));
}

Node LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return Node(std::stoi(parsed_num));
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return Node(std::stod(parsed_num));
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
Node LoadString(std::istream& input) {
    using namespace std::literals;
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return Node(s);
}

Node LoadDict(istream& input) {
    Dict result;
    char c = ' ';
    bool first = true;

    for (; input >> c && c != '}';) {
        first = false;
        if (c == ',') {
            input >> c;
        }
        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }

    if (first && c != '}') {
        throw ParsingError("ParsingError in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }

    return Node(move(result));
}

Node LoadNode(istream& input) {
    IgnoreWhiteSpace(input);

    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == '-' || std::isdigit(c)) {
        input.putback(c);
        return LoadNumber(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else {
        throw ParsingError("ParsingError in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }
}

}  // namespace

Node::Node() 
    : value_(nullptr) {
}

Node::Node(std::nullptr_t) 
    : value_(nullptr) {
}

Node::Node(int num)
    : value_(num) {
}

Node::Node(double num)
    : value_(num) {
}

Node::Node(bool v)
    : value_(v) {
}

Node::Node(Array array)
    : value_(move(array)) {
}
Node::Node(Dict map)
    : value_(move(map)) {
}

Node::Node(string value)
    : value_(move(value)) {
}

Node::Node(Value value)
    : value_(move(value)) {
}

bool Node::IsInt() const { return std::holds_alternative<int>(value_); }
bool Node::IsDouble() const { return std::holds_alternative<int>(value_) 
                                || std::holds_alternative<double>(value_); }
bool Node::IsPureDouble() const { return std::holds_alternative<double>(value_); }
bool Node::IsBool() const { return std::holds_alternative<bool>(value_); }
bool Node::IsString() const { return std::holds_alternative<std::string>(value_); }
bool Node::IsNull() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool Node::IsArray() const { return std::holds_alternative<Array>(value_); }
bool Node::IsMap() const { return std::holds_alternative<Dict>(value_); }

bool operator==(const Node& rhs, const Node& lhs) { return rhs.GetValue() == lhs.GetValue(); }
bool operator!=(const Node& rhs, const Node& lhs) { return !(rhs == lhs); }

bool operator==(const Document& rhs, const Document& lhs) { return rhs.GetRoot() == lhs.GetRoot(); }
bool operator!=(const Document& rhs, const Document& lhs) { return !(rhs == lhs); }


int Node::AsInt() const {
    try {
        return std::get<int>(value_);
    } catch(std::bad_variant_access const& e) {
        throw std::logic_error("logic_error in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }
}

bool Node::AsBool() const {
    try {
        return std::get<bool>(value_);
    } catch(std::bad_variant_access const& e) {
        throw std::logic_error("logic_error in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }
}

double Node::AsDouble() const {
    if (IsInt()) {
        return std::get<int>(value_);
    } else if (IsDouble()) {
        return std::get<double>(value_);
    } else {
        throw std::logic_error("logic_error in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }
}

const std::string& Node::AsString() const {
    try {
        return std::get<std::string>(value_);
    } catch(std::bad_variant_access const& e) {
        throw std::logic_error("logic_error in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }
}

const Array& Node::AsArray() const {
    try {
        return std::get<Array>(value_);
    } catch(std::bad_variant_access const& e) {
        throw std::logic_error("logic_error in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }
}

const Dict& Node::AsMap() const {
    try {
        return std::get<Dict>(value_);
    } catch(std::bad_variant_access const& e) {
        throw std::logic_error("logic_error in file: "s + __FILE__  + " on line: " + to_string(__LINE__));
    }
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

void PrintNode(const Node& value, const PrintContext& ctx);

// Шаблон, подходящий для вывода double, int
template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx) {
    ctx.out << value;
}

// Перегрузка функции PrintValue для вывода значений null
// В специализаци шаблона PrintValue для типа nullptr_t параметр value передаётся
// по константной ссылке, как и в основном шаблоне.
template <>
void PrintValue<std::nullptr_t>(const std::nullptr_t&, const PrintContext& ctx) {
    ctx.out << "null"sv;
}

// Перегрузка функции PrintValue для вывода значений bool
// В специализаци шаблона PrintValue для типа bool параметр value передаётся
// по константной ссылке, как и в основном шаблоне.
template <>
void PrintValue<bool>(const bool& b, const PrintContext& ctx) {
    b ? ctx.out << "true"sv : ctx.out << "false"sv;
}

// Перегрузка функции PrintValue для вывода значений string
template <>
void PrintValue<std::string>(const string& s, const PrintContext& ctx) {
    ctx.out << "\""sv;
    for (const auto& c : s) {
        if (c == '\"') {
            ctx.out << "\\\""sv;
        } else if (c == '\n') {
            ctx.out << "\\n"sv;
        } else if (c == '\r') {
            ctx.out << "\\r"sv;
        } else if (c == '\\') {
            ctx.out << "\\\\"sv;
        } else {
            ctx.out << c;
        }
    }
    ctx.out << "\""sv;
}

// Перегрузка функции PrintValue для вывода значений array
template <>
void PrintValue<Array>(const Array& arr, const PrintContext& ctx) {
    std::ostream& out = ctx.out;
    const size_t arr_size = arr.size();
    auto inner_ctx = ctx.Indented();
    if (arr_size == 0) {
        out << "[]"sv;
        return;
    }
    out << "[\n"sv;
    for (size_t i = 0; i < arr_size - 1; ++i) {
        inner_ctx.PrintIndent();
        PrintNode(arr.at(i), inner_ctx);
        out << ",\n"sv;
    }
    inner_ctx.PrintIndent();
    PrintNode(arr.at(arr_size - 1), inner_ctx);
    out.put('\n');
    ctx.PrintIndent();
    out << "]"sv;
}

// Перегрузка функции PrintValue для вывода значений Dict
template <>
void PrintValue<Dict>(const Dict& dict, const PrintContext& ctx) {
    std::ostream& out = ctx.out;
    auto inner_ctx = ctx.Indented();
    const size_t dict_size = dict.size();
    if (dict_size == 0) {
        out << "{}"sv;
        return;
    }

    out << "{\n"sv;
    bool first = true;
    for (const auto& [key, value] : dict) {
        if (!first) {
            out << ",\n"sv;
        }
        first = false;
        inner_ctx.PrintIndent();
        PrintValue(key, ctx);
        out << ": "sv;
        PrintNode(value, inner_ctx);
    }
    out.put('\n');
    ctx.PrintIndent();
    out << "}"sv;
}

const std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>& Node::GetValue() const {
    return value_;
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit(
        [&ctx](const auto& value) {
            PrintValue(value, ctx);
        },
        node.GetValue());
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), PrintContext{output});
}

}  // namespace json