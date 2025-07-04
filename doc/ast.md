# Building the AST

## Simplified Parse Tree
The simplified parse tree should ignore some punctuations, remove some non-terminals, and fold the lists.

Some regular expression notations are borrowed to express the simplified grammar:
- brackets followed immediately by a star (expression)* means the expression should appear zero or more times
- brackets followed immediately by a plus (expression)+ means the expression should appear one or more times
- brackets followed immediately by a question mark (expression)? means the expression should appear zero or one time

```
Start ::= (Decl)+

FuncDef ::= Type id ( (Param ,)* (Param)? (,)? ) BlockStmt
Param ::= Type id [ ] | Type id

VarDecl ::= Type VarAssignable (, VarAssignable)* ;
VarAssignable ::= id = Expr | id [ intconst ] | id

Var ::= id [ Var ] | id [ Constant ] | id
Type ::= int | float | str
Constant ::= intconst | floatconst | strconst

BlockStmt ::= { (VarDecl | IfStmt | WhileStmt | ForStmt | ReturnStmt | expr ; | ;)* }

expr is one of AssignExpr | OrExpr | AndExpr | EqualityExpr | RelationalExpr | SumExpr | MulExpr | UnaryExpr | FuncCall | Constant | Var

IfStmt ::= if ( expr ) BlockStmt | if ( expr ) BlockStmt else BlockStmt

WhileStmt ::= while ( expr ) BlockStmt

ForStmt ::= for ( ForVarDecl ; Expr ; Expr ) BlockStmt
ForVarDecl ::= (VarAssign (, VarAssign)*)?
VarAssign ::= Var = Expr

ReturnStmt ::= return expr ; | return ;

AssignExpr ::= Var = expr
OrExpr ::= AndExpr (|| AndExpr)+
AndExpr ::= EqualityExpr (&& EqualityExpr)+
EqualityExpr ::= RelationalExpr (EqualityOp RelationalExpr)+
RelationalExpr ::= SumExpr (RelationalOp SumExpr)+
SumExpr ::= MulExpr (SumOp MulExpr)+
MulExpr ::= UnaryExpr (MulOp UnaryExpr)+
UnaryExpr ::= UnaryOp UnaryExpr
FuncCall ::= id ( (Expr ,)* )
Factor ::= ( Expr )
```

## AST Nodes

```
Start ( declarations: AstNode[] )
FuncDef ( type: Token, id: Token, params: AstNode[], body: AstNode )
Param ( type: Token, id: Token, array: bool )
VarDecl ( type: Token, varAssignables: AstNode[] )
VarAssignable ( var: AstNode, value?: AstNode )
Var ( id: Token, arrayIndex: AstNode )
Type ( type: Token )
Constant ( value: Token )
BlockStmt ( statements: AstNode[] )
IfStmt ( condExpr: AstNode, thenBody: AstNode, elseBody?: AstNode )
WhileStmt ( condExpr: AstNode, body: AstNode )
ForStmt ( forVarDecl: AstNode[], condExpr: AstNode, incrExpr: AstNode )
VarAssign ( var: AstNode, value: AstNode )
ReturnStmt ( expr?: AstNode )
AssignExpr ( var: AstNode, expr: AstNode )
OrExpr ( lexpr: AstNode, rexpr: AstNode )
AndExpr ( lexpr: AstNode, rexpr: AstNode )
EqualExpr ( lexpr: AstNode, rexpr: AstNode )
NotEqualExpr ( lexpr: AstNode, rexpr: AstNode )
LessExpr ( lexpr: AstNode, rexpr: AstNode )
LessEqualExpr ( lexpr: AstNode, rexpr: AstNode )
GreaterExpr ( lexpr: AstNode, rexpr: AstNode )
GreaterEqualExpr ( lexpr: AstNode, rexpr: AstNode )
AddExpr ( lexpr: AstNode, rexpr: AstNode )
SubExpr ( lexpr: AstNode, rexpr: AstNode )
MulExpr ( lexpr: AstNode, rexpr: AstNode )
DivExpr ( lexpr: AstNode, rexpr: AstNode )
ModExpr ( lexpr: AstNode, rexpr: AstNode )
UnaryPlusExpr ( expr: AstNode )
UnaryMinusExpr ( expr: AstNode )
FuncCall ( id: Token, exprs: AstNode[] )
```
