/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

typedef enum TokenKind TokenKind;
enum TokenKind {
    TK_RESERVED, // 記号
    TK_IDENT,    // 変数
    TK_NUM,      // 整数
    TK_EOF,      // 入力の終わり
};

typedef struct Token Token;
struct Token {
    TokenKind kind; // トークンの種類
    Token *next;    // 次のトークン
    int val;        // 数値トークンのときはその値
    char *str;      // トークン文字列
    int len;        // トークンの長さ
};

// 抽象構文木のノード種別
typedef enum NodeKind NodeKind;
enum NodeKind {
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_NUM,     // 数値
    ND_ASSIGN,  // =
    ND_LVAR,    // ローカル変数
    ND_EQ,      // ==
    ND_NE,      // !=
    ND_LE,      // <=(左右を入れ換えることで>=にも使う)
    ND_LT,      // <(左右を入れ換えることで>にも使う)
    ND_RETURN,  // return
    ND_EXPR_STMT,  // 式文
    ND_IF,         // if文
    ND_WHILE,      // while文
};

// ローカル変数を表す型
typedef struct LVar LVar;
struct LVar {
    LVar *next;  // 次のLVarまたは終端を表すNULLが入る
    char *name;  // 変数の名前
    int len;     // 変数の名前の長さ
    int offset;  // RBPからの距離
};

typedef struct Node Node;
struct Node {
    NodeKind kind; // ノードの種類

    Node *next;    // 次の文

    Node *lhs;     // 左辺
    Node *rhs;     // 右辺

    int val;       // kindがND_NUMのときに使う

    LVar *var;     // kindがND_LVARのときに使う

    // kindがND_IFのときに使う
    Node *cond;  // ND_WHILEにも使う
    Node *then;
    Node *els;

    // kindがND_WHILEのときに使う
    Node *body;
};

typedef struct Function Function;
struct Function {
    Node *nodes;
    LVar *locals;
    int stack_size;
};

// 現在着目しているトークン
extern Token *token;
// 入力プログラム
extern char *user_input;

// utils.c
extern void error(char *fmt, ...);
extern bool startswith(char *prefix, char *str);

// parse.c
extern Token *tokenize(char *p);
extern Function* program(void);

// gen.c
extern void gencode(Function* func);
