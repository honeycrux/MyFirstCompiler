# Language Spec

## Grammar
The BNF grammar of the language is as follows.

```
Start ::= DeclList
DeclList ::= Decl DeclList | Decl
Decl ::= FuncDef | VarDecl

FuncDef ::= Type id ( ParamList ) BlockStmt

% SLR1 %
% Note 1 %
ParamList ::= Param , ParamList | Param | ε
Param ::= Type ParamVar
ParamVar ::= id [ ] | id
Type ::= int | float | str

VarDecl ::= Type VarAssignableList ;
VarAssignableList ::= VarAssignable , VarAssignableList | VarAssignable
VarAssignable ::= Var = Expr | Var

Var ::= id [ intconst ] | id
Type ::= int | float | str

BlockStmt ::= { StmtList }
StmtList ::= Stmt StmtList | ε
Stmt ::= VarDecl | IfStmt | WhileStmt | ForStmt | ReturnStmt | Expr ; | ;

IfStmt ::= if ( Expr ) BlockStmt else BlockStmt | if ( Expr ) BlockStmt

WhileStmt ::= while ( Expr ) BlockStmt

ForStmt ::= for ( ForVarDecl ; Expr ; Expr ) BlockStmt
ForVarDecl ::= VarAssignList
VarAssignList ::= VarAssign, VarAssignList | VarAssign | ε
VarAssign ::= Var = Expr

ReturnStmt ::= return Expr ; | return ;

% Note 2 %
Expr ::= AssignExpr
AssignExpr ::= Var = Expr | OrExpr
OrExpr ::= OrExpr || AndExpr | AndExpr
AndExpr ::= AndExpr && EqualityExpr | EqualityExpr
EqualityExpr ::= EqualityExpr EqualityOp RelationalExpr | RelationalExpr
RelationalExpr ::= RelationalExpr RelationalOp SumExpr | SumExpr
SumExpr ::= SumExpr SumOp MulExpr | MulExpr
MulExpr ::= MulExpr MulOp UnaryExpr | UnaryExpr
UnaryExpr ::= UnaryOp UnaryExpr | FuncCall
FuncCall ::= id ( ArgList ) | Factor
ArgList ::= Expr , ArgList | Expr | ε
Factor ::= ( Expr ) | VarConst

EqualityOp ::= == | !=
RelationalOp ::= < | <= | > | >=
SumOp ::= + | -
MulOp ::= * | / | %
UnaryOp ::= + | - | !

% LL1 %
% Note 3 %
VarConst ::= Var | Constant
Constant ::= intconst | floatconst | strconst
Var ::= id [ intconst ] | id
```

Three types of parsing are used. By default, the parsing of the grammar is done with recursive descent parsing. Grammar labelled with SLR1 and LL1 are parsed with the corresponding methods instead.

Note 1: SLR1 grammar of the ParamList grammar
```
S ::= ParamList
ParamList ::= Param , ParamList | Param | ε
Param ::= Type ParamVar
ParamVar ::= id [ ] | id
Type ::= int | float | str
```

Note 2: Non left-recursive grammar for the Expr grammar
```
Expr ::= AssignExpr
AssignExpr ::= Var = Expr | OrExpr
OrExpr ::= AndExpr OrExpr'
OrExpr' ::= || AndExpr OrExpr' | ε
AndExpr ::= EqualityExpr AndExpr'
AndExpr' ::= && EqualityExpr AndExpr' | ε
EqualityExpr ::= RelationalExpr EqualityExpr'
EqualityExpr' ::= EqualityOp RelationalExpr EqualityExpr' | ε
RelationalExpr ::= SumExpr RelationalExpr'
RelationalExpr' ::= RelationalOp SumExpr RelationalExpr' | ε
SumExpr ::= MulExpr SumExpr'
SumExpr' ::= SumOp MulExpr SumExpr' | ε
MulExpr ::= UnaryExpr MulExpr'
MulExpr' ::= MulOp UnaryExpr MulExpr' | ε
UnaryExpr ::= UnaryOp UnaryExpr | FuncCall
FuncCall ::= id ( ArgList ) | Factor
ArgList ::= Expr , ArgList | Expr | ε
Factor ::= ( Expr ) | VarConst
```

Note 3: LL1 grammar for the VarConst grammar
```
S ::= VarConst
VarConst ::= Var | Constant
Constant ::= intconst | floatconst | strconst
Var ::= id Var'
Var' ::= [ Expr ] | ε
```

## Precedence, Associativity
Operators are listed top to bottom, in descending precedence.  
l: left-to-right associativity  
r: right-to-left associativity  
My reference: https://en.cppreference.com/w/c/language/operator_precedence  
```
l function call, array subscripting () []
r unary: unary plus/minus, logical NOT + - !
l multiplication, division, remainder * / %
l addition, subtraction + -
l relational operators <, <=, >, >=
l relational operators ==, !=
l Logical AND &&
l Logical OR ||
r assignment operators =
```
