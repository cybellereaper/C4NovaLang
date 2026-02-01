grammar nova;

// --------------------------------------------------------------------
//  LEXER
// --------------------------------------------------------------------
MODULE  : 'module';
IMPORT  : 'import';
FUN     : 'fun';
LET     : 'let';
TYPE    : 'type';
IF      : 'if';
WHILE   : 'while';
ELSE    : 'else';
MATCH   : 'match';
ASYNC   : 'async';
AWAIT   : 'await';
PIPE    : '|>';
ARROW   : '->';
EFFECT  : '!';
TRUE    : 'true';
FALSE   : 'false';
NUMBER  : [0-9]+ ('.' [0-9]+)?;
STRING  : '"' (~["\\] | '\\' .)* '"' | '"""' .*? '"""';
ID      : [a-zA-Z_][a-zA-Z_0-9]*;
COMMENT : '#' ~[\r\n]* -> skip;
WS      : [ \t\r\n]+ -> skip;

// --------------------------------------------------------------------
//  PARSER
// --------------------------------------------------------------------
program
    : moduleDecl importDecl* decl* EOF
    ;

moduleDecl
    : MODULE modulePath
    ;

importDecl
    : IMPORT modulePath ('{' ID (',' ID)* '}')?
    ;

modulePath
    : ID ('.' ID)*
    ;

decl
    : typeDecl
    | funDecl
    | letDecl
    ;

typeDecl
    : TYPE ID ('=' variantList | '(' paramList? ')')
    ;

variantList
    : ID ('|' ID)*
    ;

funDecl
    : FUN ID '(' paramList? ')' (':' typeRef)? '=' expr
    ;

letDecl
    : LET ID (':' typeRef)? '=' expr
    ;

paramList
    : param (',' param)*
    ;

param
    : ID (':' typeRef)?
    ;

typeRef
    : ID
    ;

// --------------------------------------------------------------------
//  EXPRESSIONS
// --------------------------------------------------------------------
expr
    : IF expr block (ELSE block)?                 #ifExpr
    | WHILE expr block                            #whileExpr
    | MATCH expr '{' matchArm+ '}'                #matchExpr
    | ASYNC block                                 #asyncExpr
    | AWAIT expr                                  #awaitExpr
    | EFFECT expr                                 #effectExpr
    | pipeExpr                                    #pipeExpr
    ;

pipeExpr
    : callExpr (PIPE callExpr)*
    ;

callExpr
    : primary ('(' argList? ')')*
    ;

argList
    : arg (',' arg)*
    ;

arg
    : (ID '=')? expr
    ;

primary
    : literal
    | ID
    | lambda
    | '(' expr ')'
    ;

lambda
    : '(' paramList? ')' ARROW expr
    ;

// --------------------------------------------------------------------
//  MATCH / BLOCK
// --------------------------------------------------------------------
matchArm
    : ID ('(' paramList? ')')? ARROW expr
    ;

block
    : '{' exprList? '}'
    ;

exprList
    : expr (';' expr)*
    ;

// --------------------------------------------------------------------
//  LITERALS
// --------------------------------------------------------------------
literal
    : NUMBER
    | STRING
    | TRUE
    | FALSE
    | listLiteral
    ;

listLiteral
    : '[' (expr (',' expr)*)? ']'
    ;

// --------------------------------------------------------------------

