#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>

// 6.6.18: Type in the calculator example and get it to work (DONE)
// 6.6.19: Modify the calculator to report line numbers for errors (DONE)
// 6.6.20: Allow a user to define functions in the calculator (DONE)
// 6.6.21: Convert the desk calculator to use a symbol struncture instead of using the static variables number_value and string_value

using namespace std;

map<string, double> table;
// map<function_name, pair<params, function body>
map<string, pair<vector<string>, string>> function_list;
enum Token_value {
  NAME, NUMBER, FNAME, END,
  PLUS='+', MINUS='-', MUL='*', DIV='/',
  PRINT=';', ASSIGN='=', LP='(', RP=')'
};

Token_value curr_tok = PRINT;
double number_value;
string string_value;
vector<string> params;
string function_text;
int no_of_errors;
int current_line;

double expr(bool get, istream* cin);
double term(bool get, istream* cin);
double prim(bool get, istream* cin);

double error(const string& s) {
  no_of_errors++;
  cerr<< "error on line " << current_line << " : " << s << '\n';
  return 1;
}

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
      *cin >> number_value;
      return curr_tok=NUMBER;
    
    default:
      if (isalpha(ch)) {
        string_value = ch;
        while (cin -> get(ch) && isalnum(ch)) string_value.push_back(ch);
        if (ch == '(') {
          curr_tok=FNAME;
        }
        else {
          curr_tok=NAME;
        }
        cin -> putback(ch);
        return curr_tok;
      }
      error("bad token");
      return curr_tok=PRINT;
  }
}

double expr(bool get, istream* cin) {
  double left = term(get, cin);

  for(;;) {
    switch (curr_tok) {
      case PLUS:
        left += term(true, cin);
        break;
      case MINUS:
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

  function_text = functionString;
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
  params = vector<string>();
  while(paramsStream >> temp) params.push_back(temp);
  return true;
}

double process_input_stream(istream* input_stream) {
  double result;
  while(input_stream) {
    get_token(input_stream); // gets a token from cin
    if (curr_tok == END) break; // If the found token is END, break out of the loop
    if (curr_tok == PRINT) continue; // If the found token in PRINT, get more input
    result = expr(false, input_stream);
  }
  return result;
}

double prim(bool get, istream* cin) {
  if (get) get_token(cin);
  current_line++;

  switch (curr_tok) {
    case NUMBER:
    {
      double v = number_value;
      get_token(cin);
      return v;
    }
    case FNAME:
    {
      pair<vector<string>, string>& function_pair = function_list[string_value];
      if(!get_params()) return error(") expected");
      if (get_token(cin) == ASSIGN) {
        if(!begin_function_text()) return error("{ expected to begin function text");
        if(!get_function_text()) return error("} expected");
        function_pair.first = params;
        function_pair.second = function_text;
        return 0;
      }
      else {
        // temporarily add variable name and parameter values to table. Remove them after process is run
        for (int j = 0; j < params.size(); j++) table[function_pair.first[j]] = atof(params[j].c_str());

        stringstream functionStream(function_pair.second);
        double result = process_input_stream(&functionStream);
        for (string var : function_pair.first) table.erase(var);
        return result;
      }
    }

    case NAME:
    {
      double& v = table[string_value];
      if (get_token(cin) == ASSIGN) v = expr(true, cin);
      return v;
    }
    case MINUS:
      return -prim(true, cin);
    case LP:
    {
      double e = expr(true, cin);
      if (curr_tok != RP) return error(") expected");
      get_token(cin);
      return e;
    }
    default:
    return error("primary expected");
  }
}


double term(bool get, istream* cin) {
  double left = prim(get, cin);

  for(;;) {
    switch(curr_tok) {
      case MUL:
        left *= prim(true, cin);
        break;
      case DIV:
        if (double d = prim(true, cin)) {
          left /= d;
          break;
        }
        return error("divide by 0");
      default:
        return left;
    }
  }
}


int main() {
  table["pi"] = 3.1415927;
  table["e"] = 2.71828;

  cout << "TERMINAL CALCULATOR\n";

  while (cin) {
    current_line = 0;
    get_token(&cin); // gets a token from cin
    if (curr_tok == END) break; // If the found token is END, break out of the loop
    if (curr_tok == PRINT) continue; // If the found token in PRINT, get more input
    cout << expr(false, &cin) << '\n'; // Output the expression
  }

  return no_of_errors;
}