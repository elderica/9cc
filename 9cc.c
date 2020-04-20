/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TK_RESERVED, // 記号
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
    ND_NOT_EQ,  // !=
    ND_GE_EQ,   // <=(左右を入れ換えることで>=にも使う)
    ND_GE,      // <(左右を入れ換えることで>にも使う)
} NodeKind;

typedef struct Node Node;

struct Node {
    NodeKind kind; // ノードの種類
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMのときに使う
};


// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);

Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(char *p);

Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);

bool consume(char *op);
void expect(char *op);
int expect_number(void);
bool at_eof(void);

Node *expr(void);
Node *equality(void);
Node *relational(void);
Node *add(void);
Node *mul(void);
Node *unary(void);
Node *primary(void);

void gen(Node *node);

// プログラム中のどこにエラーがあるか報告する
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラーを報告するための関数
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}



// 次のトークンが期待している記号ならば、トークンを一つ読み進めて真を返す。
// そうでなければ偽を返す。
bool consume(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len   ||
        memcmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが、期待している記号ならば、トークンを1つ読み進める。
// それ以外のときは、エラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len   ||
        memcmp(token->str, op, token->len)) {
        error_at(token->str, "'%c'ではありません", op);
    }
    token = token->next;
}

// 次のトークンが数値ならば、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof(void) {
    return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
// 新しく最後尾になった新しいトークンを返す。
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// 字句解析を行なう。
Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;

    while (*p) {
        // 空白文字は読み飛ばす
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (strncmp("==", p, 2) == 0 ||
            strncmp("!=", p, 2) == 0 ||
            strncmp(">=", p, 2) == 0 ||
            strncmp("<=", p, 2) == 0 ) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
        }

        if (strchr("+-*/()<>", *p) != NULL) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (isdigit(*p)) {
            char *q = p;
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(q, &p, 10);
            cur->len = p - q;
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

// expr = equality
Node* expr(void) {
    return equality();
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality(void) {
    Node *node = relational();
    for(;;) {
        if (consume("==")) {
            node = new_node(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_node(ND_NOT_EQ, node, relational());
        } else {
            return node;
        }
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational(void) {
    Node *node = add();
    for (;;) {
        if (consume("<")) {
            node = new_node(ND_GE, node, add());
        } else if (consume("<=")) {
            node = new_node(ND_GE_EQ, node, add());
        } else if (consume(">")) {
            node = new_node(ND_GE, add(), node);    // ">"は"<"の左右を入れ換えたもの
        } else if (consume(">=")) {
            node = new_node(ND_GE_EQ, add(), node); // ">="もまた同様とみなす
        } else {
            return node;
        }
    }
}

// add = ("+" mul | "-" mul)*
Node *add(void) {
    Node *node = mul();
    for (;;) {
        if (consume("+")) {
            node = new_node(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_node(ND_SUB, node, mul());
        } else {
            return node;
        }
    }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul(void) {
    Node *node = unary();
    for (;;) {
        if (consume("*")) {
            node = new_node(ND_MUL, node, unary());
        } else if(consume("/")) {
            node = new_node(ND_DIV, node, unary());
        } else {
            return node;
        }
    }
}

// unary = ("+" | "-")? primary
Node *unary(void) {
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), unary());
    }
    return primary();
}

// primary = num | "(" expr ")"
Node *primary(void) {
    // 括弧で囲まれている場合
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    // そうでなければ数値でなければならない
    return new_node_num(expect_number());
}

void gen(Node *node) {
    if (node->kind == ND_NUM) {
        printf("  push %d\n", node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NOT_EQ:
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_GE:
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_GE_EQ:
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
    }

    printf("  push rax\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    user_input = argv[1];
    // トークナイズする
    token = tokenize(user_input);
    Node *node = expr();

    // プロローグを出力する
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    gen(node);

    printf("  pop rax\n");
    printf("  ret\n");
    return 0;
}
