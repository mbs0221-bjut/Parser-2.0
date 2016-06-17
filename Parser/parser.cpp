#include "inter.h"

// 语法分析器
class Parser{
private:
	Lexer lex;
	Token *look;
	list<map<string, Id*>*> idss;
	// 匹配Tag并预读一个词法单元
	bool match(int Tag){
		if (look->Tag == Tag){
			look = lex.scan();
			return true;
		}
		look = lex.scan();
		if (look->Tag > 255)
			printf("line[%03d]:%d not matched.\n", lex.line, look->Tag);
		else
			printf("line[%03d]:%c not matched.\n", lex.line, (char)look->Tag);
		return false;
	}
	void putid(Id *id){
		list<map<string, Id*>*>::iterator t = idss.begin();
		if ((*t)->find(((Word*)id->opt)->word) == (*t)->end()){
			(**t)[((Word*)id->opt)->word] = id;
		}
	}
	Id* getid(){
		list<map<string, Id*>*>::iterator t = idss.begin();
		for (t = idss.begin(); t != idss.end(); t++){
			if ((*t)->find(((Word*)look)->word) != (*t)->end()){
				return (*t)->at(((Word*)look)->word);
			}
		}
		printf("line%3d:%look has no definition.\n", lex.line, ((Word*)look)->word.c_str());
		return nullptr;
	}
	// 定义
	Node* def(){
		Nodes *n = new Nodes;
		Id *id;
		idss.push_front(new map<string, Id*>());
		while (look->Tag != '#'){
			switch (look->Tag){
			case ID:
				id = getid();
				switch (id->type->Tag){
				case NUM:n->Ss.push_back(stmt_assign()); break;
				case FUNCTION:n->Ss.push_back(stmt_call()); break;
				default:printf("unknown id.\n"); match(look->Tag); break;
				}
				break;
			case INT: decl_var(); break;
			case STR: decl_var(); break;
			case JSON: decl_json(); break;
			case FUNCTION: decl_func(); break;
			case PRINT:n->Ss.push_back(stmt_print()); break;
			case CIN:n->Ss.push_back(stmt_cin()); break;
			default:break;
			}
		}
		idss.front();
		idss.pop_front();
		return n;
	}
	// 声明语句
	void decl_func(){
		Function *f = new Function;
		match(FUNCTION);
		f->name = look;
		match(ID);
		Id *id = new Id((Word*)f->name, f);
		putid(id);
		match(BEGIN);
		idss.push_front(&f->V);
		while (look->Tag != END){
			switch (look->Tag){
			case INPUT:
				// 输入端口
				match(INPUT);
				match(INT);
				id = new Id((Word*)look, new Integer(0));
				putid(id);
				f->input.push_back(id);
				match(ID);
				while (look->Tag == ','){
					match(',');
					id = new Id((Word*)look, new Integer(0));
					putid(id);
					f->input.push_back(id);
					match(ID);
				}
				break;
			case OUTPUT:
				// 输出端口
				match(OUTPUT);
				match(INT);
				id = new Id((Word*)look, new Integer(0));
				putid(id);
				f->output.push_back(id);
				match(ID);
				while (look->Tag == ','){
					match(',');
					id = new Id((Word*)look, new Integer(0));
					putid(id);
					f->output.push_back(id);
					match(ID);
				}
				break;
			default:
				// 函数体
				f->body.Ss.push_back(stmt());
				break;
			}
		}
		idss.pop_front();
		match(END);
	}
	void decl_json(){
		match(JSON);
		Token* token = look;
		match(ID);
		if (look->Tag == '='){
			match('=');
			Json *json = new Json(((Word*)token)->word, stmt_access());
			putid(new Id((Word*)token, json));
		}
		else{
			Json *json = new Json(((Word*)token)->word, new JSONObject());
			putid(new Id((Word*)token, json));
			match(';');
		}
	}
	void decl_var(){
		match(INT);
		Token *token;
		while (look->Tag == ID){
			token = look;
			match(ID);
			if (look->Tag == '['){
				match('[');
				int size = ((Integer*)look)->value;
				Integer* arr = new Integer[size];
				putid(new Id((Word*)token, new Array(size, Type::Int, arr)));
				match(NUM);
				match(']');
				if (look->Tag == '{'){
					int index = 0;
					match('{');
					while (look->Tag != '}'){
						arr[index++].value = ((Integer*)look)->value;
						match(NUM);
						if (look->Tag == ',')match(',');
					}
					match('}');
				}
			}
			else{
				putid(new Id((Word*)token, Type::Int, Integer::True));
			}
			if(look->Tag == ',')match(',');
		}
		match(';');
	}
	// 执行语句
	Stmt* stmt(){
		Stmt *st = nullptr;
		Id *id;
		switch (look->Tag){
		case BEGIN:st = stmts(); break;
		case STR:decl_var(); break;
		case INT:decl_var(); break;
		case ID:
			id = getid();
			switch (id->type->Tag){
			case NUM:st = stmt_assign(); break;
			case FUNCTION:st = stmt_call(); break;
			default:printf("unknown id.\n"); match(look->Tag); break;
			}
			break;
		case IF:st = stmt_if(); break;
		case WHILE:st = stmt_while(); break;
		case DO:st = stmt_do(); break;
		case FOR:st = stmt_for(); break;
		case CASE:st = stmt_case(); break;
		case PRINT:st = stmt_print(); break;
		case CIN:st = stmt_cin(); break;
		case JSON:decl_json(); break;
		case ';':match(';'); break;
		default:match(look->Tag); break;
		}
		return st;
	}
	Stmt* stmts(){
		Stmts *sts = new Stmts();
		match(BEGIN);
		while (look->Tag != END){
			Stmt *st = stmt();
			if (st)sts->Ss.push_back(st);
		}
		match(END);
		return sts;
	}
	Stmt* stmt_call(){
		Type *t;
		Call *c = new Call;
		c->func = (Function*)getid()->type;
		match(ID);
		if (look->Tag == '('){
			// 函数调用
			Function *f = c->func;
			Function *t;
			match('(');
			vector<Id*>::iterator iter;
			for (iter = f->input.begin(); iter != f->input.end(); iter++){
				// 获取参数
				switch (look->Tag){
				case NUM:
					c->Ids.push_back(new Id(new Word(NUM, "@"), (Integer*)look));
					match(NUM);
					break;
				case ID:
					if (getid()->type->Tag == FUNCTION){
						t = (Function*)getid()->type;
						for (int i = 0; i < t->output.size(); i++){
							c->Ids.push_back(t->output[i]);// 传值
						}
					}
					else{
						c->Ids.push_back(getid());
					}
					match(ID);
					break;
				default:
					c->Ids.push_back(new Id(new Word(NUM, "@"), Integer::False));
					printf("error on params.\n");
					match(look->Tag);
					break;
				}
			}
			match(')');
		}
		return c;
	}
	Stmt* stmt_print(){
		Print *p = new Print();
		Word *w = nullptr;
		match(PRINT);
		switch (look->Tag){
		case STRING: w = (Word*)look; match(STRING); break;
		case TRUE: w = Integer::True; match(TRUE); break;
		case FALSE: w = Integer::False; match(FALSE); break;
		case ID:
			if (getid()->type->Tag == JSON)
				w = json_access();
			else
				w = expr();
			break;
		case COUNT:w = stmt_count(); break;
		case '(':w = expr(); break;
		default:p = nullptr; break;
		}
		if (w)p->Es.push_back(w);
		while (look->Tag == '.'){
			match('.');
			switch (look->Tag){
			case STRING:w = (Word*)look; match(STRING); break;
			case TRUE:w = Integer::True; match(TRUE); break;
			case FALSE:w = Integer::False; match(FALSE); break;
			case ID:
				if (getid()->type->Tag == JSON)
					w = json_access();
				else
					w = expr();
				break;
			case COUNT:w = stmt_count(); break;
			case '(':w = expr(); break;
			default:printf("error print:%n", look->Tag); p = nullptr; break;
			}
			if (w)p->Es.push_back(w);
		}
		return p;
	}
	Stmt* stmt_cin(){
		Cin *c = new Cin;
		match(CIN);
		while (look->Tag == '-'){
			match('-');
			if (getid() == nullptr)break;
			c->Ids.push_back(getid());
			match(ID);
		}
		return c;
	}
	Stmt* stmt_assign(){
		Assign *a = new Assign;
		a->E1 = getid();
		match(ID);
		match('=');
		a->E2 = expr();
		return a;
	}
	Stmt* stmt_if(){
		If *i = new If;
		match(IF);
		i->C = cond();
		match(THEN);
		idss.push_front(new map<string, Id*>());
		i->S1 = stmt();
		i->S1->V = idss.front();
		idss.pop_front();
		if (look->Tag == ELSE){
			Else *e = new Else;
			match(ELSE);
			e->C = i->C;
			e->S1 = i->S1;
			idss.push_front(new map<string, Id*>());
			e->S2 = stmt();
			e->S2->V = idss.front();
			idss.pop_front();
			return e;
		}
		return i;
	}
	Stmt* stmt_while(){
		While *w = new While;
		idss.push_front(new map<string, Id*>());
		match(WHILE);
		w->C = cond();
		match(DO);
		w->S1 = stmt();
		w->V = idss.front();
		idss.pop_front();
		return w;
	}
	Stmt* stmt_do(){
		Do *d = new Do;
		idss.push_front(new map<string, Id*>());
		match(DO);
		d->S1 = stmt();
		match(WHILE);
		d->C = cond();
		d->V = idss.front();
		idss.pop_front();
		return d;
	}
	Stmt* stmt_for(){
		For *f = new For;
		idss.push_front(new map<string, Id*>());
		match(FOR);
		f->S1 = stmt_assign();
		match(';');
		f->C = cond();
		match(';');
		f->S2 = stmt_assign();
		match(';');
		f->S3 = stmt();
		f->V = idss.front();
		idss.pop_front();
		return f;
	}
	Stmt* stmt_case(){
		Case *c = new Case;
		match(CASE);
		c->E = expr();
		while (look->Tag != END){
			Integer *i = (Integer*)look;
			match(NUM);
			match(':');
			idss.push_front(new map<string, Id*>());
			c->Ss[i->value] = stmt();
			c->Ss[i->value]->V = idss.front();
			idss.pop_front();
		}
		match(END);
		return c;
	}
	// 表达式
	Expr* cond(){
		Expr* e = expr();
		if (look->Tag == '<' || look->Tag == '=' || look->Tag == '>'){
			Token *token = look;
			match(look->Tag);
			e = new Arith(token, e, expr());
		}
		return e;
	}
	Expr* expr(){
		Expr* e = term();
		while (look->Tag == '+' || look->Tag == '-'){
			Token *token = look;
			match(look->Tag);
			e = new Arith(token, e, term());
		}
		return e;
	}
	Expr* term(){
		Expr* e = factor();
		while (look->Tag == '*' || look->Tag == '/' || look->Tag == '%'){
			Token *token = look;
			match(look->Tag);
			e = new Arith(token, e, factor());
		}
		return e;
	}
	Expr* factor()
	{
		Expr* e = nullptr;
		Function *f;
		Id *id;
		switch (look->Tag){
		case '(':  match('('); e = expr(); match(')'); break;
		case ID:
			id = getid();
			if (id->type->Tag == ARRAY){
				Locate *l = new Locate((Array*)id->type);
				match(ID);
				match('[');
				l->expr = expr();
				match(']');
				e = l;
			}
			else if (id->type->Tag == FUNCTION){
				match(ID);
				Function *f = (Function*)id->type;
				e = f->output[0];
				if (look->Tag == '['){
					match('[');
					e = f->output[((Integer*)look)->value];
					match(NUM);
					match(']');
				}
			}
			else{
				match(ID);
				e = id;
			}
			break;
		case COUNT:e = stmt_count();break;
		case NUM:e = new Constant((Integer*)look, Type::Int); match(NUM); break;
		case REAL:e = new Constant((Double*)look, Type::Double); match(REAL); break;
		default:printf("F->('%c')\n", look->Tag); match(look->Tag); break;
		}
		return e;
	}
	// JSON Access
	Expr *stmt_count(){
		Expr *e;
		match(COUNT);
		match('(');
		if (getid()->type->Tag == JSON){
			e = new Constant(new Integer(((JSONArray*)((Member*)json_access())->eval())->json.size()), Type::Int);
		}
		else
		{
			e = new Constant(new Integer(((Array*)getid()->type)->size), Type::Int); match(ID);
		}
		match(')');
		return e;
	}
	// JSON处理
	JSONValue* json_access(){
		Id *id = getid();
		match(ID);
		JSONValue *val = (JSONValue*)((Json*)id->type)->node;
		while (look->Tag == '['){
			match('[');
			switch (look->Tag){
			case ID:
			case NUM:
				val = new Access((JSONArray*)val, expr());
				break;
			case STRING:
				val = new Member((JSONObject*)val, ((Word*)look)->word);
				match(STRING);
				break;
			default:
				break;
			}
			match(']');
		}
		return val;
	}
	JSONValue* stmt_access(){
		JSONValue* node = nullptr;
		switch (look->Tag){
		case '[':node = json_array(); break;
		case '{':node = json_object(); break;
		default: break;
		}
		return node;
	}
	JSONArray* json_array(){
		JSONArray* arr = nullptr;
		if (look->Tag == '['){
			arr = new JSONArray();
			match('[');
			arr->addNode(json_object());
			while (look->Tag == ','){
				match(',');
				arr->addNode(json_object());
			}
			match(']');
		}
		return arr;
	}
	JSONObject* json_object(){
		JSONObject *obj = new JSONObject();
		if (look->Tag == '{'){
			match('{');
			JSONPair *p = json_pair();
			if (p)obj->addPair(p);
			while (look->Tag == ','){
				match(',');
				p = json_pair();
				if (p)obj->addPair(p);
			}
			match('}');
			return obj;
		}
		return nullptr;
	}
	JSONPair* json_pair(){
		if (look->Tag == STRING){
			JSONString *attr = json_string();
			match(':');
			JSONValue *value = json_attr();
			if (attr && value){
				return new JSONPair(attr, value);
			}
		}
		return nullptr;
	}
	JSONValue *json_attr(){
		JSONValue *n;
		switch (look->Tag){
		case '[':n = json_array(); break;
		case '{':n = json_object(); break;
		case STRING:n = json_string(); break;
		case NUM:n = new JSONInt(((Integer*)look)->value); match(NUM); break;
		case REAL:n = new JSONDouble(((Double*)look)->value); match(REAL); break;
		case TRUE:n = new JSONString(((Word*)look)->word); match(TRUE); break;
		case FALSE:n = new JSONString(((Word*)look)->word); match(FALSE); break;
		case NULL:n = new JSONString(((Type*)look)->word); match(NULL); break;
		default:n = nullptr; break;
		}
		return n;
	}
	JSONString *json_string(){
		JSONString *js = new JSONString(((Word*)look)->word);
		match(STRING);
		return js;
	}
public:
	Node* parse(string file){
		if (lex.open(file)){
			look = lex.scan();
			return def();
		}
		else{
			printf("can't open %look.\n", file);
			return nullptr;
		}
	}
};

// 主程序
void main()
{
	char c;
	Parser p;
	string str = "Text.txt";
	Node *look = p.parse(str);
	if (look != nullptr){
		look->eval();
	}
	cin >> c;
}