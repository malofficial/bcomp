/*----------------------------------------------------------------------
Compiler Generator Coco/R,
Copyright (c) 1990, 2004 Hanspeter Moessenboeck, University of Linz
extended by M. Loeberbauer & A. Woess, Univ. of Linz
ported to C++ by Csaba Balazs, University of Szeged
with improvements by Pat Terry, Rhodes University

This program is free software; you can redistribute it and/or modify it 
under the terms of the GNU General Public License as published by the 
Free Software Foundation; either version 2, or (at your option) any 
later version.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
for more details.

You should have received a copy of the GNU General Public License along 
with this program; if not, write to the Free Software Foundation, Inc., 
59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

As an exception, it is allowed to write an extension of Coco/R that is
used as a plugin in non-free software.

If not otherwise stated, any source code generated by Coco/R (other than 
Coco/R itself) does not fall under the GNU General Public License.
-----------------------------------------------------------------------*/


#include <wchar.h>
#include "Parser.h"
#include "Scanner.h"




void Parser::SynErr(int n) {
	if (errDist >= minErrDist) errors->SynErr(la->line, la->col, n);
	errDist = 0;
}

void Parser::SemErr(const wchar_t* msg) {
	if (errDist >= minErrDist) errors->Error(t->line, t->col, msg);
	errDist = 0;
}

void Parser::Get() {
	for (;;) {
		t = la;
		la = scanner->Scan();
		if (la->kind <= maxT) { ++errDist; break; }

		if (dummyToken != t) {
			dummyToken->kind = t->kind;
			dummyToken->pos = t->pos;
			dummyToken->col = t->col;
			dummyToken->line = t->line;
			dummyToken->next = NULL;
			coco_string_delete(dummyToken->val);
			dummyToken->val = coco_string_create(t->val);
			t = dummyToken;
		}
		la = t;
	}
}

void Parser::Expect(int n) {
	if (la->kind==n) Get(); else { SynErr(n); }
}

void Parser::ExpectWeak(int n, int follow) {
	if (la->kind == n) Get();
	else {
		SynErr(n);
		while (!StartOf(follow)) Get();
	}
}

bool Parser::WeakSeparator(int n, int syFol, int repFol) {
	if (la->kind == n) {Get(); return true;}
	else if (StartOf(repFol)) {return false;}
	else {
		SynErr(n);
		while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
			Get();
		}
		return StartOf(syFol);
	}
}

void Parser::B() {
		ParseList=L"(B ";scopepos=0;
		while (la->kind == _identifier) {
			Definition();
		}
		Expect(_EOF);
		ParseList+=L")";std::wcout << ParseList; 
}

void Parser::Definition() {
		Name name; bool isarray; int position;
		isarray=false;
		while (!(la->kind == _EOF || la->kind == _identifier)) {SynErr(67); Get();}
		position=ParseList.length(); 
		Ident(name);
		if (IsDeclaredGlobal(name))  AlreadyErr(name) ; 
		ParseList.append(name);ParseList.append(L" ");
		if (la->kind == 12 /* "(" */) {
			FunctionDefinition();
			ParseList.insert(position,L"(FUNCDEF " ); ParseList.append(L") " ); 
		} else if (StartOf(1)) {
			ArraySize(isarray);
			ParseList.insert(position,isarray?L"(GARRDEF ":L"(GVARDEF ") ; ParseList.append(L") " ); 
		} else SynErr(68);
}

void Parser::Ident(Name name) {
		Expect(_identifier);
		wcscpy(name, t->val) ; 
}

void Parser::GotoLabel(Name name) {
		Expect(_gotolabel);
		wcscpy(name, t->val) ; 
		Expect(7 /* ":" */);
}

void Parser::ConstVal() {
		Name name; 
		if (la->kind == _number) {
			
			Get();
			wcscpy(name, t->val) ; ParseList.append(L"(INT "); 
			ParseList.append(name);ParseList.append(L")") ;
		} else if (la->kind == _string) {
			Get();
			wcscpy(name, t->val) ;ParseList.append(L"(STRING `"); 
			ParseList.append(name);ParseList.append(L"`)") ;
		} else if (la->kind == _char) {
			Get();
			wcscpy(name, t->val) ;ParseList.append(L"(CHAR `"); 
			ParseList.append(name);ParseList.append(L"`)") ;
		} else if (la->kind == _onumber) {
			Get();
			wcscpy(name, t->val) ;ParseList.append(L"(ONUMBER "); 
			ParseList.append(name);ParseList.append(L")") ;
		} else SynErr(69);
}

void Parser::FunctionDefinition() {
		scopepos=ParseList.length(); 
		FunctionHeader();
		FunctionBody();
}

void Parser::ArraySize(bool & isarray) {
		int position; 
		isarray=false; 
		if (la->kind == 8 /* "[" */) {
			Get();
			if (la->kind == _number) {
				Get();
				isarray=true; ParseList.append(L"(ASIZE "); 
				ParseList.append(t->val);ParseList.append(L")") ;
			}
			Expect(9 /* "]" */);
		}
		if (StartOf(2)) {
			position=ParseList.length(); 
			Initializator();
			ParseList.insert(position,L"(INIT " ); ParseList.append(L") " ); 
			while (la->kind == 10 /* "," */) {
				Get();
				position=ParseList.length();isarray=true; 
				Initializator();
				ParseList.insert(position,L"(INIT " ); ParseList.append(L") " ); 
			}
		}
		Expect(11 /* ";" */);
}

void Parser::Initializator() {
		Name name;  
		if (StartOf(3)) {
			ConstVal();
		} else if (la->kind == _identifier) {
			Ident(name);
			if (!IsDeclaredGlobal(name)) UndecErr(name); ParseList.append(L"(SAMEAS "); 
			ParseList.append(name);ParseList.append(L")") ;
		} else SynErr(70);
}

void Parser::FunctionHeader() {
		Expect(12 /* "(" */);
		ParseList.append(L"(FHEADER "); 
		if (la->kind == _identifier) {
			FormalParamList();
		}
		ParseList.append(L") " ); 
		Expect(13 /* ")" */);
}

void Parser::FunctionBody() {
		
		Statement();
}

void Parser::FormalParamList() {
		FormalParameter();
		while (la->kind == 10 /* "," */) {
			Get();
			FormalParameter();
		}
}

void Parser::Statement() {
		bool hasdeclarations; int position; 
		while (!(StartOf(4))) {SynErr(71); Get();}
		position=ParseList.length();hasdeclarations=false; 
		while (StartOf(5)) {
			if (la->kind == _gotolabel || la->kind == 14 /* "case" */ || la->kind == 15 /* "default" */) {
				Label();
				hasdeclarations=true; 
			} else if (la->kind == 26 /* "extrn" */) {
				ExtrnDeclaration();
				hasdeclarations=true; 
			} else {
				AutoDeclaration();
				hasdeclarations=true; 
			}
		}
		switch (la->kind) {
		case _identifier: case _number: case _onumber: case _string: case _char: case 12 /* "(" */: case 48 /* "&" */: case 57 /* "+" */: case 58 /* "-" */: case 59 /* "*" */: case 62 /* "~" */: case 63 /* "!" */: case 64 /* "++" */: case 65 /* "--" */: {
			StatementExpression();
			break;
		}
		case 16 /* "break" */: {
			BreakStatement();
			break;
		}
		case 17 /* "{" */: {
			CompoundStatement();
			break;
		}
		case 19 /* "continue" */: {
			ContinueStatement();
			break;
		}
		case 20 /* "goto" */: {
			GotoStatement();
			break;
		}
		case 21 /* "if" */: {
			IfStatement();
			break;
		}
		case 11 /* ";" */: {
			NullStatement();
			break;
		}
		case 23 /* "return" */: {
			ReturnStatement();
			break;
		}
		case 24 /* "switch" */: {
			SwitchStatement();
			break;
		}
		case 25 /* "while" */: {
			WhileStatement();
			break;
		}
		default: SynErr(72); break;
		}
		if (hasdeclarations) { 
		ParseList.insert(position,L"(DECLSTAT " ); ParseList.append(L") " );} 
}

void Parser::FormalParameter() {
		Name name;  
		Ident(name);
		if (IsDeclaredLocal(name)) AlreadyErr(name); 
		ParseList.append(L"(FPARAM "); 
		ParseList.append(name);ParseList.append(L" ");;ParseList.append(L")") ;
}

void Parser::Label() {
		Name name;int position; 
		if (la->kind == 14 /* "case" */) {
			position=ParseList.length(); 
			Get();
			ConstVal();
			Expect(7 /* ":" */);
			ParseList.insert(position,L"(CASE " ); ParseList.append(L" ) " ); 
		} else if (la->kind == 15 /* "default" */) {
			Get();
			Expect(7 /* ":" */);
			ParseList.append(L"(DEFAULT) "); 
		} else if (la->kind == _gotolabel) {
			GotoLabel(name);
			ParseList.append(L"(LABEL ");ParseList.append(name); ;ParseList.append(L" )");
		} else SynErr(73);
}

void Parser::ExtrnDeclaration() {
		Name name;  
		Expect(26 /* "extrn" */);
		Ident(name);
		ParseList.append(L"(EXTRN ");ParseList.append(name); ;ParseList.append(L" )");
		while (la->kind == 10 /* "," */) {
			Get();
			Ident(name);
			ParseList.append(L"(EXTRN ");ParseList.append(name); ;ParseList.append(L" )");
		}
		Expect(11 /* ";" */);
}

void Parser::AutoDeclaration() {
		Name name; int position; bool isarray;
		Expect(27 /* "auto" */);
		isarray=false;position=ParseList.length(); 
		Ident(name);
		ParseList.append(name);ParseList.append(L" "); 
		if (la->kind == _number || la->kind == 8 /* "[" */) {
			if (la->kind == 8 /* "[" */) {
				Get();
				Expect(_number);
				isarray=true; ParseList.append(L"(ASIZE "); 
				ParseList.append(t->val);ParseList.append(L")") ;
				Expect(9 /* "]" */);
			} else {
				Get();
				ParseList.append(L"(INIT "); 
				ParseList.append(t->val);ParseList.append(L")") ;
			}
		}
		ParseList.insert(position,isarray?L"(LARRDEF ":L"(LVARDEF ") ; ParseList.append(L") " ); 
		while (la->kind == 10 /* "," */) {
			Get();
			isarray=false;position=ParseList.length(); 
			Ident(name);
			ParseList.append(name);ParseList.append(L" ");  
			if (la->kind == _number || la->kind == 8 /* "[" */) {
				if (la->kind == 8 /* "[" */) {
					Get();
					Expect(_number);
					isarray=true; ParseList.append(L"(ASIZE"); 
					ParseList.append(t->val);ParseList.append(L")") ;
					Expect(9 /* "]" */);
				} else {
					Get();
					ParseList.append(L"(INIT"); 
					ParseList.append(t->val);ParseList.append(L")") ;
				}
			}
			ParseList.insert(position,isarray?L"(LARRDEF ":L"(LVARDEF ") ; ParseList.append(L") " ); 
		}
		Expect(11 /* ";" */);
}

void Parser::StatementExpression() {
		Expression();
		ExpectWeak(11 /* ";" */, 6);
}

void Parser::BreakStatement() {
		Expect(16 /* "break" */);
		ParseList.append(L"(BREAK) "); 
		ExpectWeak(11 /* ";" */, 6);
}

void Parser::CompoundStatement() {
		Expect(17 /* "{" */);
		ParseList.append(L"(BLOCK "); 
		while (StartOf(7)) {
			Statement();
		}
		Expect(18 /* "}" */);
		ParseList.append(L") "); 
}

void Parser::ContinueStatement() {
		Expect(19 /* "continue" */);
		ParseList.append(L"(CONTINUE) "); 
		ExpectWeak(11 /* ";" */, 6);
}

void Parser::GotoStatement() {
		Name name;  
		Expect(20 /* "goto" */);
		Ident(name);
		ParseList.append(L"(GOTO ");ParseList.append(name); ;ParseList.append(L")");
		Expect(11 /* ";" */);
}

void Parser::IfStatement() {
		int position; bool haselse;
		haselse=false; 
		Expect(21 /* "if" */);
		position=ParseList.length(); 
		Expect(12 /* "(" */);
		Expression();
		Expect(13 /* ")" */);
		Statement();
		if (la->kind == 22 /* "else" */) {
			Get();
			Statement();
			haselse=true; 
		}
		ParseList.insert(position,haselse? L"(IFELSE ": L"(IF ");ParseList.append(L")" );  
}

void Parser::NullStatement() {
		Expect(11 /* ";" */);
}

void Parser::ReturnStatement() {
		int position; bool paramExists; 
		paramExists = false; 
		Expect(23 /* "return" */);
		if (la->kind == 12 /* "(" */) {
			ExpectWeak(12 /* "(" */, 4);
			Expression();
			paramExists = true; 
			ExpectWeak(13 /* ")" */, 4);
		}
		ParseList.insert(position, paramExists? L"(RETURNPARAM " : L"(RETURN "); ParseList.append(L")"); 
		Expect(11 /* ";" */);
}

void Parser::SwitchStatement() {
		int position; 
		position=ParseList.length(); 
		Expect(24 /* "switch" */);
		Expect(12 /* "(" */);
		Expression();
		Expect(13 /* ")" */);
		CompoundStatement();
		ParseList.insert(position, L"(SWITCH "); ParseList.append(L")"); 
}

void Parser::WhileStatement() {
		int position; 
		position=ParseList.length(); 
		Expect(25 /* "while" */);
		Expect(12 /* "(" */);
		Expression();
		Expect(13 /* ")" */);
		Statement();
		ParseList.insert(position, L"(WHILE "); ParseList.append(L")"); 
}

void Parser::Expression() {
		AssignExpr();
}

void Parser::AssignExpr() {
		int position; 
		position=ParseList.length(); 
		CondExpr();
		if (StartOf(8)) {
			switch (la->kind) {
			case 28 /* "=" */: {
				Get();
				AssignExpr();
				if (!(Assignable(position))) { SemErr(L"Not assignable"); }
				ParseList.insert(position,L"(MOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 29 /* "=*" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(MULTMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 30 /* "=/" */: {
				Get();
				AssignExpr();
				if (!(Assignable(position))) { SemErr(L"Not assignable"); }
				ParseList.insert(position,L"(DIVMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 31 /* "=%" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(MODMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 32 /* "=+" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(ADDMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 33 /* "=-" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(SUBMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 34 /* "=&" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(ANDMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 35 /* "=^" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(XORMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 36 /* "=|" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(ORMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 37 /* "=<<" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(LSHIFTMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 38 /* "=>>" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(RSHIFTMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 39 /* "=<" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(ISLESSMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 40 /* "=<=" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(ISLESSEQUMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 41 /* "=>" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(ISGREATERMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 42 /* "=>=" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(ISGREATEREQUMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 43 /* "===" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(ISEQUMOV " ); 
				ParseList.append(L") " );
				break;
			}
			case 44 /* "=!=" */: {
				Get();
				AssignExpr();
				ParseList.insert(position,L"(ISNEQUMOV " ); 
				ParseList.append(L") " );
				break;
			}
			}
		}
}

void Parser::CondExpr() {
		int position; 
		position=ParseList.length(); 
		OrExpr();
		if (la->kind == 45 /* "?" */) {
			Get();
			Expression();
			Expect(7 /* ":" */);
			CondExpr();
			ParseList.insert(position,L"(CONDEXPR " ); 
			ParseList.append(L") " );
		}
}

void Parser::OrExpr() {
		int position; 
		position=ParseList.length(); 
		XorExpr();
		while (la->kind == 46 /* "|" */) {
			Get();
			XorExpr();
			ParseList.insert(position,L"(OR " ); 
			ParseList.append(L") " );
		}
}

void Parser::XorExpr() {
		int position; 
		position=ParseList.length(); 
		AndExpr();
		while (la->kind == 47 /* "^" */) {
			Get();
			AndExpr();
			ParseList.insert(position,L"(XOR " ); 
			ParseList.append(L") " );
		}
}

void Parser::AndExpr() {
		int position; 
		position=ParseList.length(); 
		EqlExpr();
		while (la->kind == 48 /* "&" */) {
			Get();
			EqlExpr();
			ParseList.insert(position,L"(AND " ); 
			ParseList.append(L") " );
		}
}

void Parser::EqlExpr() {
		int position; 
		position=ParseList.length(); 
		RelExpr();
		while (la->kind == 49 /* "==" */ || la->kind == 50 /* "!=" */) {
			if (la->kind == 49 /* "==" */) {
				Get();
				RelExpr();
				ParseList.insert(position,L"(EQU " ); 
				ParseList.append(L") " );
			} else {
				Get();
				RelExpr();
				ParseList.insert(position,L"(NEQU " ); 
				ParseList.append(L") " );
			}
		}
}

void Parser::RelExpr() {
		int position; 
		position=ParseList.length(); 
		ShiftExpr();
		while (StartOf(9)) {
			if (la->kind == 51 /* "<" */) {
				Get();
				ShiftExpr();
				ParseList.insert(position,L"(LESSTHAN " ); 
				ParseList.append(L") " );
			} else if (la->kind == 52 /* ">" */) {
				Get();
				ShiftExpr();
				ParseList.insert(position,L"(GREATERTHAN " ); 
				ParseList.append(L") " );
			} else if (la->kind == 53 /* "<=" */) {
				Get();
				ShiftExpr();
				ParseList.insert(position,L"(LESSEQUTHAN " ); 
				ParseList.append(L") " );
			} else {
				Get();
				ShiftExpr();
				ParseList.insert(position,L"(GREATEREQUTHAN " ); 
				ParseList.append(L") " );
			}
		}
}

void Parser::ShiftExpr() {
		int position; 
		position=ParseList.length(); 
		AddExpr();
		while (la->kind == 55 /* "<<" */ || la->kind == 56 /* ">>" */) {
			if (la->kind == 55 /* "<<" */) {
				Get();
				AddExpr();
				ParseList.insert(position,L"(LSHIFT " ); 
				ParseList.append(L") " );
			} else {
				Get();
				AddExpr();
				ParseList.insert(position,L"(RSHIFT " ); 
				ParseList.append(L") " );
			}
		}
}

void Parser::AddExpr() {
		int position; 
		position=ParseList.length(); 
		MultExpr();
		while (la->kind == 57 /* "+" */ || la->kind == 58 /* "-" */) {
			if (la->kind == 57 /* "+" */) {
				Get();
				MultExpr();
				ParseList.insert(position,L"(ADD " ); 
				ParseList.append(L") " );
			} else {
				Get();
				MultExpr();
				ParseList.insert(position,L"(SUB " ); 
				ParseList.append(L") " );
			}
		}
}

void Parser::MultExpr() {
		int position; 
		position=ParseList.length(); 
		UnaryExpr();
		while (la->kind == 59 /* "*" */ || la->kind == 60 /* "/" */ || la->kind == 61 /* "%" */) {
			if (la->kind == 59 /* "*" */) {
				Get();
				UnaryExpr();
				ParseList.insert(position,L"(MUL " ); 
				ParseList.append(L") " );
			} else if (la->kind == 60 /* "/" */) {
				Get();
				UnaryExpr();
				ParseList.insert(position,L"(DIV " ); 
				ParseList.append(L") " );
			} else {
				Get();
				UnaryExpr();
				ParseList.insert(position,L"(MOD " ); 
				ParseList.append(L") " );
			}
		}
}

void Parser::UnaryExpr() {
		int position; 
		position=ParseList.length(); 
		switch (la->kind) {
		case _identifier: case _number: case _onumber: case _string: case _char: case 12 /* "(" */: {
			PostfixExpr();
			break;
		}
		case 48 /* "&" */: {
			Get();
			ParseList.insert(position,L"(ADDROF "); 
			UnaryExpr();
			ParseList.append(L")"); 
			break;
		}
		case 59 /* "*" */: {
			Get();
			ParseList.insert(position,L"(PTR "); 
			UnaryExpr();
			ParseList.append(L")"); 
			break;
		}
		case 57 /* "+" */: {
			Get();
			ParseList.insert(position,L"(UPLUS "); 
			UnaryExpr();
			ParseList.append(L")"); 
			break;
		}
		case 58 /* "-" */: {
			Get();
			ParseList.insert(position,L"(UMINUS "); 
			UnaryExpr();
			ParseList.append(L")"); 
			break;
		}
		case 62 /* "~" */: {
			Get();
			ParseList.insert(position,L"(UNEG "); 
			UnaryExpr();
			ParseList.append(L")"); 
			break;
		}
		case 63 /* "!" */: {
			Get();
			ParseList.insert(position,L"(UNOT "); 
			UnaryExpr();
			ParseList.append(L")"); 
			break;
		}
		case 64 /* "++" */: {
			Get();
			ParseList.insert(position,L"(PREINC "); 
			UnaryExpr();
			ParseList.append(L")"); 
			break;
		}
		case 65 /* "--" */: {
			Get();
			ParseList.insert(position,L"(PREDEC "); 
			UnaryExpr();
			ParseList.append(L")"); 
			break;
		}
		default: SynErr(74); break;
		}
}

void Parser::PostfixExpr() {
		int position; 
		position=ParseList.length(); 
		Primary();
		while (StartOf(10)) {
			if (la->kind == 8 /* "[" */) {
				Get();
				Expression();
				Expect(9 /* "]" */);
				ParseList.insert(position,L"(INDEX " ); 
				ParseList.append(L") " );
			} else if (la->kind == 12 /* "(" */) {
				Get();
				if (StartOf(11)) {
					ArgExprList();
				}
				Expect(13 /* ")" */);
				ParseList.insert(position,L"(FUNCCALL " ); 
				ParseList.append(L") " );
			} else if (la->kind == 64 /* "++" */) {
				Get();
				ParseList.insert(position,L"(POSTINC " ); 
				ParseList.append(L") " );
			} else {
				Get();
				ParseList.insert(position,L"(POSTDEC " ); 
				ParseList.append(L") " );
			}
		}
}

void Parser::Primary() {
		Name name; int position;  
		if (la->kind == _identifier) {
			Ident(name);
			ParseList.append(L"(VAR " );ParseList.append(name );ParseList.append(L")" );
		} else if (StartOf(3)) {
			ConstVal();
		} else if (la->kind == 12 /* "(" */) {
			Get();
			Expression();
			Expect(13 /* ")" */);
		} else SynErr(75);
}

void Parser::ArgExprList() {
		int position; 
		position=ParseList.length(); 
		AssignExpr();
		ParseList.insert(position,L"(ARG " ); 
		ParseList.append(L") " );
		while (la->kind == 10 /* "," */) {
			Get();
			position=ParseList.length(); 
			AssignExpr();
			ParseList.insert(position,L"(ARG " ); 
			ParseList.append(L") " );
		}
}




// If the user declared a method Init and a mehtod Destroy they should
// be called in the contructur and the destructor respctively.
//
// The following templates are used to recognize if the user declared
// the methods Init and Destroy.

template<typename T>
struct ParserInitExistsRecognizer {
	template<typename U, void (U::*)() = &U::Init>
	struct ExistsIfInitIsDefinedMarker{};

	struct InitIsMissingType {
		char dummy1;
	};
	
	struct InitExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static InitIsMissingType is_here(...);

	// exist only if ExistsIfInitIsDefinedMarker is defined
	template<typename U>
	static InitExistsType is_here(ExistsIfInitIsDefinedMarker<U>*);

	enum { InitExists = (sizeof(is_here<T>(NULL)) == sizeof(InitExistsType)) };
};

template<typename T>
struct ParserDestroyExistsRecognizer {
	template<typename U, void (U::*)() = &U::Destroy>
	struct ExistsIfDestroyIsDefinedMarker{};

	struct DestroyIsMissingType {
		char dummy1;
	};
	
	struct DestroyExistsType {
		char dummy1; char dummy2;
	};

	// exists always
	template<typename U>
	static DestroyIsMissingType is_here(...);

	// exist only if ExistsIfDestroyIsDefinedMarker is defined
	template<typename U>
	static DestroyExistsType is_here(ExistsIfDestroyIsDefinedMarker<U>*);

	enum { DestroyExists = (sizeof(is_here<T>(NULL)) == sizeof(DestroyExistsType)) };
};

// The folloing templates are used to call the Init and Destroy methods if they exist.

// Generic case of the ParserInitCaller, gets used if the Init method is missing
template<typename T, bool = ParserInitExistsRecognizer<T>::InitExists>
struct ParserInitCaller {
	static void CallInit(T *t) {
		// nothing to do
	}
};

// True case of the ParserInitCaller, gets used if the Init method exists
template<typename T>
struct ParserInitCaller<T, true> {
	static void CallInit(T *t) {
		t->Init();
	}
};

// Generic case of the ParserDestroyCaller, gets used if the Destroy method is missing
template<typename T, bool = ParserDestroyExistsRecognizer<T>::DestroyExists>
struct ParserDestroyCaller {
	static void CallDestroy(T *t) {
		// nothing to do
	}
};

// True case of the ParserDestroyCaller, gets used if the Destroy method exists
template<typename T>
struct ParserDestroyCaller<T, true> {
	static void CallDestroy(T *t) {
		t->Destroy();
	}
};

void Parser::Parse() {
	t = NULL;
	la = dummyToken = new Token();
	la->val = coco_string_create(L"Dummy Token");
	Get();
	B();
	Expect(0);
}

Parser::Parser(Scanner *scanner) {
	maxT = 66;

	ParserInitCaller<Parser>::CallInit(this);
	dummyToken = NULL;
	t = la = NULL;
	minErrDist = 2;
	errDist = minErrDist;
	this->scanner = scanner;
	errors = new Errors();
}

bool Parser::StartOf(int s) {
	const bool T = true;
	const bool x = false;

	static bool set[12][68] = {
		{T,T,T,T, T,T,T,x, x,x,x,T, T,x,T,T, T,T,x,T, T,T,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,T,T,T, x,x,T,T, T,T,x,x},
		{x,T,x,T, T,T,T,x, T,x,x,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x},
		{x,T,x,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x},
		{x,x,x,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x},
		{T,T,T,T, T,T,T,x, x,x,x,T, T,x,T,T, T,T,x,T, T,T,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,T,T,T, x,x,T,T, T,T,x,x},
		{x,x,T,x, x,x,x,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, x,x,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x},
		{T,T,T,T, T,T,T,x, x,x,x,T, T,x,T,T, T,T,T,T, T,T,T,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,T,T,T, x,x,T,T, T,T,x,x},
		{x,T,T,T, T,T,T,x, x,x,x,T, T,x,T,T, T,T,x,T, T,T,x,T, T,T,T,T, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,T,T,T, x,x,T,T, T,T,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,T,T, T,T,T,T, T,T,T,T, T,T,T,T, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x},
		{x,x,x,x, x,x,x,x, T,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,T,x,x},
		{x,T,x,T, T,T,T,x, x,x,x,x, T,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, T,x,x,x, x,x,x,x, x,T,T,T, x,x,T,T, T,T,x,x}
	};



	return set[s][la->kind];
}

Parser::~Parser() {
	ParserDestroyCaller<Parser>::CallDestroy(this);
	delete errors;
	delete dummyToken;
}

Errors::Errors() {
	count = 0;
}

void Errors::SynErr(int line, int col, int n) {
	wchar_t* s;
	switch (n) {
			case 0: s = coco_string_create(L"EOF expected"); break;
			case 1: s = coco_string_create(L"identifier expected"); break;
			case 2: s = coco_string_create(L"gotolabel expected"); break;
			case 3: s = coco_string_create(L"number expected"); break;
			case 4: s = coco_string_create(L"onumber expected"); break;
			case 5: s = coco_string_create(L"string expected"); break;
			case 6: s = coco_string_create(L"char expected"); break;
			case 7: s = coco_string_create(L"\":\" expected"); break;
			case 8: s = coco_string_create(L"\"[\" expected"); break;
			case 9: s = coco_string_create(L"\"]\" expected"); break;
			case 10: s = coco_string_create(L"\",\" expected"); break;
			case 11: s = coco_string_create(L"\";\" expected"); break;
			case 12: s = coco_string_create(L"\"(\" expected"); break;
			case 13: s = coco_string_create(L"\")\" expected"); break;
			case 14: s = coco_string_create(L"\"case\" expected"); break;
			case 15: s = coco_string_create(L"\"default\" expected"); break;
			case 16: s = coco_string_create(L"\"break\" expected"); break;
			case 17: s = coco_string_create(L"\"{\" expected"); break;
			case 18: s = coco_string_create(L"\"}\" expected"); break;
			case 19: s = coco_string_create(L"\"continue\" expected"); break;
			case 20: s = coco_string_create(L"\"goto\" expected"); break;
			case 21: s = coco_string_create(L"\"if\" expected"); break;
			case 22: s = coco_string_create(L"\"else\" expected"); break;
			case 23: s = coco_string_create(L"\"return\" expected"); break;
			case 24: s = coco_string_create(L"\"switch\" expected"); break;
			case 25: s = coco_string_create(L"\"while\" expected"); break;
			case 26: s = coco_string_create(L"\"extrn\" expected"); break;
			case 27: s = coco_string_create(L"\"auto\" expected"); break;
			case 28: s = coco_string_create(L"\"=\" expected"); break;
			case 29: s = coco_string_create(L"\"=*\" expected"); break;
			case 30: s = coco_string_create(L"\"=/\" expected"); break;
			case 31: s = coco_string_create(L"\"=%\" expected"); break;
			case 32: s = coco_string_create(L"\"=+\" expected"); break;
			case 33: s = coco_string_create(L"\"=-\" expected"); break;
			case 34: s = coco_string_create(L"\"=&\" expected"); break;
			case 35: s = coco_string_create(L"\"=^\" expected"); break;
			case 36: s = coco_string_create(L"\"=|\" expected"); break;
			case 37: s = coco_string_create(L"\"=<<\" expected"); break;
			case 38: s = coco_string_create(L"\"=>>\" expected"); break;
			case 39: s = coco_string_create(L"\"=<\" expected"); break;
			case 40: s = coco_string_create(L"\"=<=\" expected"); break;
			case 41: s = coco_string_create(L"\"=>\" expected"); break;
			case 42: s = coco_string_create(L"\"=>=\" expected"); break;
			case 43: s = coco_string_create(L"\"===\" expected"); break;
			case 44: s = coco_string_create(L"\"=!=\" expected"); break;
			case 45: s = coco_string_create(L"\"?\" expected"); break;
			case 46: s = coco_string_create(L"\"|\" expected"); break;
			case 47: s = coco_string_create(L"\"^\" expected"); break;
			case 48: s = coco_string_create(L"\"&\" expected"); break;
			case 49: s = coco_string_create(L"\"==\" expected"); break;
			case 50: s = coco_string_create(L"\"!=\" expected"); break;
			case 51: s = coco_string_create(L"\"<\" expected"); break;
			case 52: s = coco_string_create(L"\">\" expected"); break;
			case 53: s = coco_string_create(L"\"<=\" expected"); break;
			case 54: s = coco_string_create(L"\">=\" expected"); break;
			case 55: s = coco_string_create(L"\"<<\" expected"); break;
			case 56: s = coco_string_create(L"\">>\" expected"); break;
			case 57: s = coco_string_create(L"\"+\" expected"); break;
			case 58: s = coco_string_create(L"\"-\" expected"); break;
			case 59: s = coco_string_create(L"\"*\" expected"); break;
			case 60: s = coco_string_create(L"\"/\" expected"); break;
			case 61: s = coco_string_create(L"\"%\" expected"); break;
			case 62: s = coco_string_create(L"\"~\" expected"); break;
			case 63: s = coco_string_create(L"\"!\" expected"); break;
			case 64: s = coco_string_create(L"\"++\" expected"); break;
			case 65: s = coco_string_create(L"\"--\" expected"); break;
			case 66: s = coco_string_create(L"??? expected"); break;
			case 67: s = coco_string_create(L"this symbol not expected in Definition"); break;
			case 68: s = coco_string_create(L"invalid Definition"); break;
			case 69: s = coco_string_create(L"invalid ConstVal"); break;
			case 70: s = coco_string_create(L"invalid Initializator"); break;
			case 71: s = coco_string_create(L"this symbol not expected in Statement"); break;
			case 72: s = coco_string_create(L"invalid Statement"); break;
			case 73: s = coco_string_create(L"invalid Label"); break;
			case 74: s = coco_string_create(L"invalid UnaryExpr"); break;
			case 75: s = coco_string_create(L"invalid Primary"); break;

		default:
		{
			wchar_t format[20];
			coco_swprintf(format, 20, L"error %d", n);
			s = coco_string_create(format);
		}
		break;
	}
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	coco_string_delete(s);
	count++;
}

void Errors::Error(int line, int col, const wchar_t *s) {
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
	count++;
}

void Errors::Warning(int line, int col, const wchar_t *s) {
	wprintf(L"-- line %d col %d: %ls\n", line, col, s);
}

void Errors::Warning(const wchar_t *s) {
	wprintf(L"%ls\n", s);
}

void Errors::Exception(const wchar_t* s) {
	wprintf(L"%ls", s); 
	exit(1);
}


