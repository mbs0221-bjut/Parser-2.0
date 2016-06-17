#include "lexer.h"

// 表达式
struct Expr :Word{
	Token *opt;// 常数，标识符或运算符
	Type *type;// 运算类型
	int label;
	static int nlabel;
	Expr(Token *opt, Type *type) :opt(opt), type(type){ label = nlabel++; }
	virtual Type *eval(){
		return type;
	}
};

int Expr::nlabel = 0;

// ID
struct Id :Expr{
	Type *value;
	Id(Word *word, Type *type, Type *value) :Expr(word, type), value(value){}
	virtual Type *eval(){
		return value;
	}
};

struct Constant :Expr{
	static Constant *Null;
	Constant(Token *token, Type *type) :Expr(token, type){  }
	virtual Type *eval(){ 
		return (Type*)opt;
	}
};

Constant *Constant::Null = new Constant(Type::Int, new Integer(NULL));

// 算术表达式
struct Arith :Expr{
	Expr *E1, *E2;
	Arith(Token *opt, Expr *e1, Expr *e2) :Expr(opt, Type::max(e1->type, e2->type)), E1(e1), E2(e2){  }
	virtual Type *eval(){
		Type *result;
		if (type == Type::Int){
			Integer *a = (Integer*)E1->eval();
			Integer *b = (Integer*)E2->eval();
			switch (opt->Tag){
			case '+':result = new Integer(a->value+b->value); break;
			case '-':result = new Integer(a->value - b->value); break;
			case '*':result = new Integer(a->value*b->value); break;
			case '/':result = new Integer(a->value / b->value); break;
			case '%':result = new Integer(a->value%b->value); break;
			case '&':result = new Integer(a->value&b->value); break;
			case '>':result = a->value > b->value ? Integer::True : Integer::False; break;
			case '<':result = a->value < b->value ? Integer::True : Integer::False; break;
			case '=':result = a->value == b->value ? Integer::True : Integer::False; break;
			default:result = Integer::False; printf("unsupported int option '%c'\n", opt->Tag); break;
			}
			result->eval();
		}else if (type == Type::Double){
			Double *a = (Double*)E1->eval();
			Double *b = (Double*)E2->eval();
			switch (opt->Tag){
			case '+':result = new Double(a->value+b->value); break;
			case '-':result = new Double(a->value - b->value); break;
			case '*':result = new Double(a->value*b->value); break;
			case '/':result = new Double(a->value / b->value); break;
			case '>':result = a->value > b->value ? Integer::True : Integer::False; break;
			case '<':result = a->value < b->value ? Integer::True : Integer::False; break;
			case '=':result = a->value == b->value ? Integer::True : Integer::False; break;
			default:result = Integer::False; printf("unsupported double option '%c'\n", opt->Tag); break;
			}
			result->eval();
		}else{
			result = Integer::True;
			printf("result:%d\n", type->Tag);
		}
		return result;
	}
};

struct Locate :Expr{
	Expr *expr;
	Locate(Array *arr):Expr(new Token(INDEX), arr){  }
	virtual Integer *eval(){
		Integer *i = (Integer*)expr->eval();;
		return &((Integer*)((Array*)type)->value)[i->value];
	}
};

// Access
struct Access :JSONObject {
	JSONArray *A1;
	Expr *E1;
	Access(JSONArray* A1, Expr *E1) :A1(A1), E1(E1){  }
	virtual JSONObject* eval(){
		Integer* e1 = (Integer*)E1->eval();
		JSONObject *obj = A1->eval()->json.at(e1->value);
		obj->eval();
		word = obj->word;
		return obj;
	}
};

// Access
struct Member :JSONValue {
	JSONObject *A1;
	string str;
	Member(JSONObject* A1, string str) :JSONValue(A1->width), A1(A1), str(str){  }
	virtual JSONValue* eval(){
		JSONObject *obj = A1->eval();
		word = (*obj)[str]->eval()->word;
		return (*obj)[str];
	}
};

struct Node{
	virtual void eval(){  }
};

struct Nodes :Node{
	list<Node*> Ss;
	virtual void eval(){
		list<Node*>::iterator iter;
		for (iter = Ss.begin(); iter != Ss.end(); iter++){
			(*iter)->eval();
		}
	}
};

struct Stmt :Node{
	map<string, Id*>* V;
	virtual void eval(){
	}
};

struct Stmts :Stmt{
	list<Stmt*> Ss;
	virtual void eval(){
		list<Stmt*>::iterator iter;
		for (iter = Ss.begin(); iter != Ss.end(); iter++){
			(*iter)->eval();
		}
	}
};

struct Function : Type{
	Token *name;
	Stmts body;
	vector<Id*> input;
	vector<Id*> output;
	map<string, Id*> V;
	Function(){ Tag = FUNCTION; }
	virtual Type* eval(){
		body.eval();
		return this;
	}
};

struct Assign :Stmt{
	Id *E1;
	Expr *E2;
	virtual void eval(){
		E1->type = E2->eval();
		E1->eval();
	}
};

struct If :Stmt{
	Expr *C;
	Stmt *S1;
	virtual void eval(){
		if (C->eval() == Integer::True){
			S1->eval();
		}
	}
};

struct Else :Stmt{
	Expr *C;
	Stmt *S1;
	Stmt *S2;
	virtual void eval(){
		if (C->eval() == Integer::True){
			S1->eval();
		}
		else{
			S2->eval();
		}
	}
};

struct While :Stmt{
	Expr *C;
	Stmt *S1;
	virtual void eval(){
		while (C->eval() == Integer::True){
			S1->eval();
		}
	}
};

struct Do :Stmt{
	Expr *C;
	Stmt *S1;
	virtual void eval(){
		do{
			S1->eval();
		} while (C->eval() == Integer::True);
	}
};

struct For :Stmt{
	Stmt *S1;
	Expr *C;
	Stmt *S2;
	Stmt *S3;
	virtual void eval(){
		S1->eval();
		while (C->eval() == Integer::True){
			S3->eval();
			S2->eval();
		}
	}
};

struct Case :Stmt{
	Expr *E;
	map<int, Stmt*> Ss;
	virtual void eval(){
		Integer *Ret = (Integer*)E->eval();
		Ss[Ret->value]->eval();
	}
};

struct Print :Stmt{
	list<Word*> Es;
	virtual void eval(){
		list<Word*>::iterator iter;
		for (iter = Es.begin(); iter != Es.end(); iter++){
			Word *Ret = (*iter)->eval();
			Ret = Ret->eval();
			printf("%s", Ret->word.c_str());
		}
	}
};

struct Cin :Stmt{
	list<Id*> Ids;
	virtual void eval(){
		list<Id*>::iterator iter;
		for (iter = Ids.begin(); iter != Ids.end(); iter++){
			switch ((*iter)->type->Tag){
			case NUM:cin >> ((Integer*)(*iter)->type)->value; break;
			case INT:cin >> ((Integer*)(*iter)->type)->value; break;
			case REAL:cin >> ((Double*)(*iter)->type)->value; break;
			case DOUBLE:cin >> ((Double*)(*iter)->type)->value; break;
			default:break;
			}
		}
	}
};

struct Call : Stmt{
	vector<Id*> Ids;
	Function *func;
	virtual void eval(){
		for (int i = 0; i < Ids.size(); i++){
			func->input[i]->type = Ids[i]->type;// 传值
		}
		func->eval();
		for (int i = 0; i < Ids.size(); i++){
			Ids[i]->type = func->input[i]->type;// 传引用，改变原值
		}
	}
};