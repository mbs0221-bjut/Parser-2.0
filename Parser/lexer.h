#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <list>
#include <vector>
#include <stack>
#include <sstream>
#include <map>

using namespace std;

//宏定义种别码
enum Tag{
	IF = 256, THEN, ELSE, DO, WHILE, FOR, CASE, PRINT, BASIC,
	ID, NUM, REAL, STRING, COUNT, CIN, ARRAY, INDEX,
	INT, DOUBLE, STR, JSON, BEGIN, END, FUNCTION, LAMBDA,
	AND, OR, EQ, NE, GE, BE, NEG, TRUE, FALSE, INPUT, OUTPUT, CALL,
	INC, DEC, SHL, SHR, TEMP
};

//二元组
struct Token{
	int Tag;		//种别码
	Token(){
		Tag = NULL;
	}
	Token(int Tag) :Tag(Tag){  }
};

struct Word :Token{
	string word;	//字符
	static Word *Temp;
	Word(){
		this->Tag = NULL;
		this->word = "";
	}
	Word(int Tag, string word) :Token(Tag){
		this->word = word;
	}
	virtual Word *eval(){
		return this;
	}
};

Word *Word::Temp = new Word(TEMP, "TEMP");

struct Type :Word{
	int width;
	static Type *Int, *Char, *Double, *Float, *Bool;
	Type(){ Tag = BASIC; word = "type", width = 0; }
	Type(Word word, int width) :Word(word), width(width){}
	Type(int Tag, string word, int width) :Word(Tag, word), width(width){}
	static Type *max(Type *T1, Type *T2){
		return T1->width > T2->width ? T1 : T2;
	}
	virtual Type *eval(){
		return this;
	}
};

Type *Type::Int = new Type(INT, "int", 4);
Type *Type::Double = new Type(DOUBLE, "double", 16);

// 整数
struct Integer :Type{
	int value;		//数字
	static Integer *True, *False;
	Integer() : value(-1){ Tag = NUM; width = 4; }
	Integer(int val) : value(val){ Tag = NUM; width = 4; }
	virtual Type *eval(){
		ostringstream oss;
		oss << value;
		word = oss.str();
		return this;
	}
};

Integer *Integer::True = new Integer(1);
Integer *Integer::False = new Integer(0);

// 小数
struct Double :Type{
	double value;
	Double(double value) : value(value){ Tag = REAL; width = 16; }
	virtual Type *eval(){
		ostringstream oss;
		oss << value;
		word = oss.str();
		return this;
	}
};

// String
struct String :Type{
	String(string value) :Type(STRING, value, value.length()){  }
	virtual Type *eval(){
		return this;
	}
};

struct Array :Type{
	int size;
	Type *value;
	Type *type;
	Array(int size, Type *type, Type *value) : size(size), type(type), value(value){ Tag = ARRAY; }
	virtual Type *eval(){
		return this;
	}
};

// JSON
struct JSONValue :Type{
	static JSONValue *Null;
	JSONValue(int width) :Type(JSON, "", width){ *eval(); }
	virtual JSONValue *eval(){
		return this;
	}
};

JSONValue *JSONValue::Null = new JSONValue(0);

struct JSONString :JSONValue{
	string str;
	JSONString(string str) :JSONValue(str.length()), str(str){ *eval(); }
	virtual JSONString *eval(){
		ostringstream fout;
		fout << "\"" << str << "\"";
		word = fout.str();
		return this;
	}
};

struct JSONInt :JSONValue{
	int num;
	JSONInt(int num) :JSONValue(4), num(num){ *eval(); }
	virtual JSONInt *eval(){
		ostringstream fout;
		fout << num;
		word = fout.str();
		return this;
	}
};

struct JSONDouble :JSONValue{
	double Double;
	JSONDouble(double d) :JSONValue(16), Double(d){ *eval(); }
	virtual JSONDouble *eval(){
		ostringstream fout;
		fout << Double;
		word = fout.str();
		return this;
	}
};

struct JSONPair : JSONValue{
	JSONString *name;
	JSONValue* node;
	JSONPair() :JSONValue(node->width){  }
	JSONPair(JSONString *name, JSONValue *node) :JSONValue(node->width), name(name), node(node){  }
	virtual JSONValue *eval(){
		ostringstream fout;
		fout << name->eval()->word << ":" << node->eval()->word;
		word = fout.str();
		return this;
	}
};

struct JSONObject :JSONValue{
	vector<JSONPair*> json;
	static JSONObject *Null;
	JSONObject() :JSONValue(0){ json.clear(); *eval(); }
	void addPair(JSONPair *p){
		json.push_back(p);
		width += p->width;
	}
	JSONValue* operator [](string name){
		vector<JSONPair*>::iterator it;
		for (it = json.begin(); it != json.end(); it++){
			if ((*it)->name->str == name){
				return (*it)->node;
			}
		}
		printf("no attr named %s.\n", name.c_str());
		return JSONValue::Null;
	}
	virtual JSONObject *eval(){
		ostringstream fout;
		fout << "{";
		for (JSONPair* node : json){
			if (node){
				if (node != json[0]){
					fout << ",";
				}
				fout << node->eval()->word;
			}
		}
		fout << "}";
		word = fout.str();
		return this;
	}
};

JSONObject *JSONObject::Null = new JSONObject();

struct JSONArray :JSONValue{
	vector<JSONObject*> json;
	JSONArray() :JSONValue(0){ json.clear(); *eval(); }
	void addNode(JSONObject *p){
		width += p->width;
		json.push_back(p);
	}
	JSONObject* operator [](int i){
		json.shrink_to_fit();
		if (i >= json.size()){
			printf("this size is %d.", json.size());
			return JSONObject::Null;
		}
		return json[i];
	}
	virtual JSONArray *eval(){
		ostringstream fout;
		fout << "[";
		for (JSONObject* node : json){
			if (node != json[0]){
				fout << ",";
			}
			fout << node->eval()->word;
		}
		fout << "]";
		word = fout.str();
		return this;
	}
};

struct Json :Type{
	string name;
	Type* node;
	Json(string name, Type* node) :Type(JSON, "", node->width), name(name), node(node){ *eval(); }
	virtual Json *eval(){
		ostringstream fout;
		fout << "{" << name << ":" << node->eval()->word << "}";
		word = fout.str();
		return this;
	}
};

// 词法分析器
class Lexer{
private:
	char peek;
	map<string, Word*> words;
	ifstream inf;// 文件输入流
	bool read(char c){
		char a;
		inf.read(&a, sizeof(char));
		return a == c;
	}
public:
	int column = 1;
	int line = 1;
	Lexer(){
		words["int"] = Type::Int;// new Word(INT, "int");
		words["double"] = Type::Double;// new Word(INT, "int");
		words["if"] = new Word(IF, "if");
		words["then"] = new Word(THEN, "then");
		words["else"] = new Word(ELSE, "else");
		words["while"] = new Word(WHILE, "while");
		words["do"] = new Word(DO, "do");
		words["for"] = new Word(FOR, "for");
		words["case"] = new Word(CASE, "case");
		words["print"] = new Word(PRINT, "print");
		words["begin"] = new Word(BEGIN, "begin");
		words["end"] = new Word(END, "end");
		words["true"] = Integer::True;
		words["false"] = Integer::False;
		words["null"] = new Word(NULL, "null");
		words["json"] = new Word(JSON, "json");
		words["function"] = new Word(FUNCTION, "function");
		words["input"] = new Word(INPUT, "input");
		words["output"] = new Word(OUTPUT, "output"); 
		words["call"] = new Word(CALL, "call");
		words["count"] = new Word(COUNT, "count");
		words["cin"] = new Word(CIN, "cin");
		words["str"] = new Word(STR, "str");
	}
	~Lexer(){
		words.clear();
		inf.close();
	}
	bool open(string file){
		inf.open(file, ios::in);
		if (inf.is_open()){
			return true;
		}
		return false;
	}
	Token *scan(){//LL(1)
		if (inf.eof()){
			return new Token(EOF);
		}
		while (inf.read(&peek, 1)){
			column++;
			if (peek == ' ' || peek == '\t')continue;
			else if (peek == '\n'){ column = 0; line++; }
			else if (peek == '/'){
				Token *t;
				if (t = skip_comment()){
					return t;
				}
			}
			else break;
		}
		if (peek == '"'){// "THIS IS A TEST"
			return match_string();
		}
		if (peek == '\''){
			return match_char();
		}
		if (isalpha(peek) || peek == '_'){// 
			return match_id();//a _
		}
		if (isdigit(peek)){
			if (peek == '0'){
				inf.read(&peek, 1);
				if (peek == 'x'){
					return match_hex();
				}
				else if (isdigit(peek) && peek >= '1'&&peek <= '7'){
					return match_oct();
				}
				inf.seekg(-1, ios_base::cur);// adsa+...
				return new Integer(0);
			}
			else{
				return match_number();
			}
		}
		return match_other();
	}// a, b, c, int;
	Token *match_string(){
		string str;
		inf.read(&peek, 1);//
		while (peek != '"'){
			if (peek == '\\'){
				inf.read(&peek, 1);// "\""
				switch (peek){
				case 'a':str.push_back('\a'); break;
				case 'b':str.push_back('\b'); break;
				case 'f':str.push_back('\f'); break;
				case 'n':str.push_back('\n'); break;
				case 'r':str.push_back('\r'); break;
				case 't':str.push_back('\t'); break;
				case 'v':str.push_back('\v'); break;
				case '\\':str.push_back('\\'); break;
				case '\'':str.push_back('\''); break;
				case '\"':str.push_back('"'); break;
				case '?':str.push_back('\?'); break;
				case '0':str.push_back('\0'); break;
				default:str.push_back('\\'); str.push_back(peek); break;
				}
			}
			else{
				str.push_back(peek);
			}
			inf.read(&peek, 1);
		}
		return new Word(STRING, str);
	}
	Token *match_char(){
		char c; // '
		inf.read(&peek, 1);
		if (peek == '\\'){// '\a
			inf.read(&peek, 1);
			switch (peek){
			case 'a':c = '\a'; break;
			case 'b':c = '\b'; break;
			case 'f':c = '\f'; break;
			case 'n':c = '\n'; break;
			case 'r':c = '\r'; break;
			case 't':c = '\t'; break;
			case 'v':c = '\v'; break;
			case '\\':c = '\\'; break;
			case '\'':c = '\''; break;
			case '\"':c = '"'; break;
			case '?':c = '\?'; break;
			case '0':c = '\0'; break;
			default:c = '\\'; c = peek; break;
			}
			inf.read(&peek, 1);// '\a'
		}else{
			c = peek;
		}
		return new Integer(c);
	}
	Token *match_id(){
		string str;
		while (isalnum(peek) || peek == '_'){
			str.push_back(peek);
			inf.read(&peek, 1);
		}
		inf.seekg(-1, ios_base::cur);// adsa+...
		map<string, Word*>::iterator iter;
		iter = words.find(str);
		if (iter == words.end()){
			return new Word(ID, str);// a1211
		}
		else{
			return (*iter).second;// 
		}
	}
	Token *match_number(){
		int ivalue = 0;
		while (isdigit(peek)){
			ivalue = 10 * ivalue + peek - '0';// 123e-3
			inf.read(&peek, 1);
		}
		if (peek == 'e' || peek == 'E'){// 123e3
			double dvalue = ivalue;
			int positive = 1;
			int index = 0;
			inf.read(&peek, 1);
			if (peek == '-'){
				positive = -1;
			}
			else if (peek == '+'){
				positive = 1;
			}
			else {
				inf.seekg(-1, ios_base::cur);
			}
			inf.read(&peek, 1);
			while (isdigit(peek)){
				index = 10 * index + peek - '0';
				inf.read(&peek, 1);
			}
			inf.seekg(-1, ios_base::cur);
			if (positive == 1){
				while (index-- > 0) dvalue *= 10;
			}
			else{
				while (index-- > 0) dvalue /= 10;
			}
			return new Double(dvalue);
		}
		if (peek == '.'){// 23.89
			double dvalue = ivalue;
			double power = 1;
			inf.read(&peek, 1);
			while (isdigit(peek)){
				power *= 10;
				dvalue = dvalue + (peek - '0') / power;
				inf.read(&peek, 1);
			}
			if (peek == 'f' || peek == 'F'){
				return new Double(dvalue);
			}
			if (peek == 'e' || peek == 'E'){
				int positive = 1;
				int index = 0;
				inf.read(&peek, 1);
				if (peek == '-'){
					positive = -1;
					inf.read(&peek, 1);
				}
				if (peek == '+'){
					inf.read(&peek, 1);
				}
				while (isdigit(peek)){
					index = 10 * index + peek - '0';
					inf.read(&peek, 1);
				}
				inf.seekg(-1, ios_base::cur);
				if (positive == 1){
					while (index-- > 0) dvalue *= 10;
				}
				else{
					while (index-- > 0) dvalue /= 10;
				}
				return new Double(dvalue);
			}
			inf.seekg(-1, ios_base::cur);
			return new Double(dvalue);
		}
		inf.seekg(-1, ios_base::cur);
		return new Integer(ivalue);
	}
	Token *match_other(){
		return new Token(peek);
	}
	Token *skip_comment(){
		if (peek == '/'){
			inf.read(&peek, 1);
			if (peek == '*'){
				do{
					inf.read(&peek, 1);
					if (peek == '\n')line++;
					if (peek == '*'){
						inf.read(&peek, 1);/**/
						if (peek == '\n')line++;
						if (peek == '/')break;
					}
				} while (peek != EOF);
				return nullptr;
			}
			else if (peek == '/')
			{
				while (!read('\n'));
				inf.seekg(-1, ios_base::cur);
				return nullptr;
			}
			else
			{
				inf.seekg(-1, ios_base::cur);
			}
			return new Token('/');
		}
		return nullptr;
	}
	Token *match_hex(){
		int num = 0;
		do{
			inf.read(&peek, 1);
			if (isdigit(peek)){
				num = 16 * num + peek - '0';
			}
			else if (tolower(peek) >= 'a'&&tolower(peek) <= 'f'){
				num = 16 * num + 10 + tolower(peek) - 'a';
			}
			else{
				inf.seekg(-1, ios_base::cur);// adsa+...
				return new Integer(num);
			}
		} while (true);
	}
	Token *match_oct(){
		int num = 0;
		if (peek >= '1'&&peek <= '7'){
			num = 8 * num + peek - '0';
			inf.read(&peek, 1);
			do{
				if (peek >= '0'&&peek <= '7'){
					num = 8 * num + peek - '0';
				}
				else{
					inf.seekg(-1, ios_base::cur);// adsa+...
					return new Integer(num);
				}
				inf.read(&peek, 1);
			} while (true);
		}
		else{
			inf.seekg(-1, ios_base::cur);// adsa+...
			return new Integer(num);
		}
	}
};