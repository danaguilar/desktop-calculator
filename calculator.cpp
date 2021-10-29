#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>

// 8.5.9: Modify the desk calculator to use exceptions. Keep a record of the mistakes you make. Suggest ways of avoiding such mistakes in the future

// 8.5.10: Write plus(), minus(), multiply(), and divide() functions that check for possible overflow and underflow and that throw exceptions if such errors happen

// 8.5.11: Modify the calculator to use the above functions

using namespace std;

// Interfaces
namespace Parser {
  double expr(bool get, istream* cin);
  double term(bool get, istream* cin);
  double prim(bool get, istream* cin);
}

namespace Lexer {
  enum Token_value {
    NAME, NUMBER, FNAME, END,
    PLUS='+', MINUS='-', MUL='*', DIV='/',
    PRINT=';', ASSIGN='=', LP='(', RP=')'
  };

  Token_value curr_tok = PRINT;
  Token_value get_token(istream*);
}

namespace SymbolTable {
  map<string, double> table;
  map<string, pair<vector<string>, string>> function_list;
  double number_value;
  string string_value;
  vector<string> params;
  string function_text;
}

namespace Errors {
  struct Divide_By_Zero { };
  struct Syntax_Error {
    std::string message;
    Syntax_Error(std::string msg) {
      message = msg;
    }
  };
  int no_of_errors;
  int current_line;
  double error(const string&);
}

// Implementations
namespace Errors {
  double error(const string& s) {
    no_of_errors++;
    cerr<< "error on line " << current_line << " : " << s << '\n';
    return 1;
  }
}

namespace Lexer {

  Token_value get_token(istream* cin) {
  char ch;

    do {
      if(!cin -> get(ch)) return curr_tok = END;
    } while (ch != '\n' && isspace(ch)); // skips all whitespace except for the endline

    // From here, ch should exist and be a non-whitespace character or the endline character
    switch(ch) {
      case 0:
        return curr_tok=END;
      case ':':
      case '\n':
        return curr_tok=PRINT;

      case ';': case '*': case '/': case '+': case '-':
      case '(': case ')': case '=':
        return curr_tok=Token_value(ch);
      
      case '0': case '1': case '2': case '3': case '4': 
      case '5': case '6': case '7': case '8': case '9': 
      case '.':
        cin ->putback(ch);
        *cin >> SymbolTable::number_value;
        return curr_tok=NUMBER;
      
      default:
        if (isalpha(ch)) {
          SymbolTable::string_value = ch;
          while (cin -> get(ch) && isalnum(ch)) SymbolTable::string_value.push_back(ch);
          if (ch == '(') {
            curr_tok=FNAME;
          }
          else {
            curr_tok=NAME;
          }
          cin -> putback(ch);
          return curr_tok;
        }
        Errors::error("bad token");
        return curr_tok=PRINT;
    }
  }

}

// Implementation
namespace Parser {
  double expr(bool get, istream* cin) {
    double left = term(get, cin);

    for(;;) {
      switch (Lexer::curr_tok) {
        case Lexer::PLUS:
          left += term(true, cin);
          break;
        case Lexer::MINUS:
          left -= term(true, cin);
          break;
        default:
          return left;
      }
    }
  }

  bool begin_function_text() {
    char ch;
    do {
      if(!cin.get(ch)) return false;
    } while(isspace(ch));

    if(ch != '{') return false;
    return true;
  }

  bool get_function_text() {
    string functionString;

    char ch;
    do {
      if(!cin.get(ch)) return false;
      if(ch != '}') functionString.push_back(ch);
    } while(ch != '}');

    SymbolTable::function_text = functionString;
    return true;
  }

  bool get_params() {
    string paramsString;

    char ch;
    // pull string until RP
    do {
      if(!cin.get(ch)) return false;
      if(!isspace(ch) && ch != ')' && ch != '(') paramsString.push_back(ch);
    } while(ch != ')');

    // pull names out of string and place them into params
    replace(paramsString.begin(), paramsString.end(), ',',' ');
    stringstream  paramsStream(paramsString);
    string temp;
    SymbolTable::params = vector<string>();
    while(paramsStream >> temp) SymbolTable::params.push_back(temp);
    return true;
  }

  double process_input_stream(istream* input_stream) {
    double result;
    while(input_stream) {
      Lexer::get_token(input_stream); // gets a token from cin
      if (Lexer::curr_tok == Lexer::END) break; // If the found token is END, break out of the loop
      if (Lexer::curr_tok == Lexer::PRINT) continue; // If the found token in PRINT, get more input
      result = expr(false, input_stream);
    }
    return result;
  }

  double prim(bool get, istream* cin) {
    if (get) Lexer::get_token(cin);
    Errors::current_line++;

    switch (Lexer::curr_tok) {
      case Lexer::NUMBER:
      {
        double v = SymbolTable::number_value;
        Lexer::get_token(cin);
        return v;
      }
      case Lexer::FNAME:
      {
        pair<vector<string>, string>& function_pair = SymbolTable::function_list[SymbolTable::string_value];
        if(!get_params()) throw Errors::Syntax_Error(") expected");
        if (Lexer::get_token(cin) == Lexer::ASSIGN) {
          if(!begin_function_text()) throw Errors::Syntax_Error("{ expected to begin function text");
          if(!get_function_text()) throw Errors::Syntax_Error("} expected");
          function_pair.first = SymbolTable::params;
          function_pair.second = SymbolTable::function_text;
          return 0;
        }
        else {
          // temporarily add variable name and parameter values to table. Remove them after process is run
          for (int j = 0; j < SymbolTable::params.size(); j++) SymbolTable::table[function_pair.first[j]] = atof(SymbolTable::params[j].c_str());

          stringstream functionStream(function_pair.second);
          double result = process_input_stream(&functionStream);
          for (string var : function_pair.first) SymbolTable::table.erase(var);
          return result;
        }
      }

      case Lexer::NAME:
      {
        double& v = SymbolTable::table[SymbolTable::string_value];
        if (Lexer::get_token(cin) == Lexer::ASSIGN) v = expr(true, cin);
        return v;
      }
      case Lexer::MINUS:
        return -prim(true, cin);
      case Lexer::LP:
      {
        double e = expr(true, cin);
        if (Lexer::curr_tok != Lexer::RP) throw Errors::Syntax_Error(") expected");
        Lexer::get_token(cin);
        return e;
      }
      default:
        throw Errors::Syntax_Error("primary expected");
    }
  }

  double term(bool get, istream* cin) {
    double left = prim(get, cin);

    for(;;) {
      switch(Lexer::curr_tok) {
        case Lexer::MUL:
          left *= prim(true, cin);
          break;
        case Lexer::DIV:
          if (double d = prim(true, cin)) {
            left /= d;
            break;
          }
          throw Errors::Divide_By_Zero();
        default:
          return left;
      }
    }
  }
}



int main() {
  SymbolTable::table["pi"] = 3.1415927;
  SymbolTable::table["e"] = 2.71828;

  cout << "TERMINAL CALCULATOR\n";

  while (cin) {
    try {
      Errors::current_line = 0;
      Lexer::get_token(&cin); // gets a token from cin
      if (Lexer::curr_tok == Lexer::END) break; // If the found token is END, break out of the loop
      if (Lexer::curr_tok == Lexer::PRINT) continue; // If the found token in PRINT, get more input
      cout << Parser::expr(false, &cin) << '\n'; // Output the expression
    }
    catch(Errors::Divide_By_Zero error) {
      cout << "Error on line " << Errors::current_line <<": Cannot divide by 0\n";
    }
    catch(Errors::Syntax_Error error) {
      cout << "Error on line " << Errors::current_line <<": " << error.message << endl;
    }
  }

  return Errors::no_of_errors;
}