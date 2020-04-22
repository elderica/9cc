/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 変数
    TK_NUM,      // 整数
    TK_EOF,      // 入力の終わり
} TokenKind;

typedef struct Token Token;
struct Token {
    TokenKind kind; // トークンの種類
    Token *next;    // 次のトークン
    int val;        // 数値トークンのときはその値
    char *str;      // トークン文字列
    int len;        // トークンの長さ
};

// 抽象構文木のノード種別
typedef enum {
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_NUM,     // 数値
    ND_EQ,      // ==
    ND_NE,      // !=
    ND_LE,      // <=(左右を入れ換えることで>=にも使う)
    ND_LT,      // <(左右を入れ換えることで>にも使う)
} NodeKind;

typedef struct Node Node;
struct Node {
    NodeKind kind; // ノードの種類
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMのときに使う
};

// 現在着目しているトークン
extern Token *token;
// 入力プログラム
extern char *user_input;

// container.c
extern void error(char *fmt, ...);
extern bool startswith(char *prefix, char *str);

// parse.c
extern void error_at(char *loc, char *fmt, ...);
extern Token *new_token(TokenKind kind, Token *cur, char *str, int len);
extern Token *tokenize(char *p);

extern Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
extern Node *new_node_num(int val);

extern bool consume(char *op);
extern void expect(char *op);
extern int expect_number(void);
extern bool at_eof(void);

extern Node *expr(void);
extern Node *equality(void);
extern Node *relational(void);
extern Node *add(void);
extern Node *mul(void);
extern Node *unary(void);
extern Node *primary(void);

// gen.c
extern void gen(Node *node);

