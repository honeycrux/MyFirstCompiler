# Language Spec

## Grammar

```
Start ::= DeclList
DeclList ::= Decl DeclList | Decl
Decl ::= FuncDef | VarDecl

FuncDef ::= Type id ( ParamList ) BlockStmt

% SLR1 %
ParamList ::= Param, ParamList | Param | ε
Param ::= Type Var
Var ::= id [] | id

VarDecl ::= Type VarList ;
VarList ::= VarAssignable , VarList | VarAssignable
VarAssigable ::= Var = Expr | Var

Var ::= id [ Expr ] | id
Type ::= int | float | str

BlockStmt ::= { StmtList }
StmtList ::= Stmt StmtList | ε
Stmt ::= VarDecl | IfStmt | WhileStmt | ForStmt | ReturnStmt | Expr ; | ; | BlockStmt

IfStmt ::= if ( Expr ) BlockStmt | if ( Expr ) BlockStmt else BlockStmt

WhileStmt ::= while ( Expr ) BlockStmt

ForStmt ::= for ( ForVarDecl ; Expr ; ExprList ) BlockStmt
ForVarDecl ::= Type VarList | VarList
ExprList ::= Expr , ExprList | Expr | ε

ReturnStmt ::= return Expr ; | return ;

% LL1 %
Expr ::= AssignExpr
AssignExpr ::= Var = AssignExpr | OrExpr
OrExpr ::= OrExpr || AndExpr | AndExpr
AndExpr ::= AndExpr && EqualityExpr | EqualityExpr
EqualityExpr ::= EqualityExpr EqualityOp CompareExpr | CompareExpr
EqualityOp ::= == | !=
CompareExpr ::= CompareExpr CompareOp SumExpr | SumExpr
CompareOp ::= < | <= | > | >=
SumExpr ::= SumExpr SumOp MulExpr | MulExpr
SumOp ::= + | -
MulExpr ::= MulExpr MulOp UnaryExpr
MulOp ::= * | ?
UnaryExpr ::= UnaryOp UnaryExpr | FuncCall
FuncCall ::= id ( ArgList ) | Factor
ArgList ::= ArgList, Exp | Exp | ε
Factor ::= ( Expr ) | Var | Constant
Constant ::= intconst | floatconst | strconst
Var ::= id [ intconst ] | id
```

## Precedence, Associativity
Operators are listed top to bottom, in descending precedence.  
l: left-to-right associativity  
r: right-to-left associativity  
My reference: https://en.cppreference.com/w/c/language/operator_precedence  
```
l function call, array subscripting () []
r unary: unary plus/minus, logical NOT - !
l multiplication, division, remainder * / %
l addition, subtraction + -
l relational operators <, <=, >, >=
l relational operators ==, !=
l Logical AND &&
l Logical OR ||
r assignment operators =
```
