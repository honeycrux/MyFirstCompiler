# Building the AST

## Simplified Parse Tree
Should ignore most punctuations but keep operators.

```
Start ::= [Decl]+

FuncDef ::= Type id ( [Param ,]+ ) BlockStmt
Param ::= Type id [ ] | Type id

VarDecl ::= Type [VarAssignable ,]+ ;
VarAssignable ::= Var = Expr | Var

Var ::= id [ intconst ] | id
Type ::= int | float | str
Constant ::= intconst | floatconst | strconst

BlockStmt ::= { [VarDecl | IfStmt | WhileStmt | ForStmt | ReturnStmt | expr ; | ; ]+ }

expr is one of AssignExpr | OrExpr | AndExpr | EqualityExpr | RelationalExpr | SumExpr | MulExpr | UnaryExpr | FuncCall | Constant | Var

IfStmt ::= if ( expr ) BlockStmt | if ( expr ) BlockStmt else BlockStmt

WhileStmt ::= while ( expr ) BlockStmt

ForStmt ::= for ( ForVarDecl ; expr ; expr ) BlockStmt
ForVarDecl ::= [VarAssignable ,]+

ReturnStmt ::= return expr ; | return ;

AssignExpr ::= Var = expr
OrExpr ::= AndExpr [|| AndExpr]+
AndExpr ::= EqualityExpr [&& EqualityExpr]+
EqualityExpr ::= RelationalExpr [EqualityOp RelationalExpr]+
RelationalExpr ::= SumExpr [RelationalOp SumExpr]+
SumExpr ::= MulExpr [SumOp MulExpr]+
MulExpr ::= UnaryExpr [MulOp UnaryExpr]+
UnaryExpr ::= UnaryOp UnaryExpr
FuncCall ::= id ( [Expr ,]+ )
Factor ::= ( Expr )
```

## AST Nodes

```
Start ( declarations: AstNode[] )
FuncDef ( type: Token, id: Token, params: AstNode[], body: AstNode )
Param ( type: Token, id: Token, array: bool )
VarDecl ( type: Token, varAssignables: AstNode[] )
VarAssignable ( var: AstNode, value?: AstNode )
Var ( id: Token, arraySize?: Token )
Type ( type: Token )
Constant ( value: Token )
BlockStmt ( statements: AstNode[] )
IfStmt ( condExpr: AstNode, thenBody: AstNode, elseBody?: AstNode )
WhileStmt ( condExpr: AstNode, body: AstNode )
ForStmt ( forVarDecl: AstNode[], condExpr: AstNode, incrExpr: AstNode )
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
