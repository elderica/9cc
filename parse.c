/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include "9cc.h"

// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

// 各行(式の並び)の構文解析結果を保存する配列
Node *code[MAX_LINES+1];

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

// 次のトークンが識別子ならば、そのトークンを返してトークンを一つ読み進める。
// そうでなければNULLを返す。
Token *consume_ident(void) {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *cur = token;
    token = token->next;
    return cur;
}

// 次のトークンが、期待している記号ならば、トークンを1つ読み進める。
// それ以外のときは、エラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED ||
        strlen(op) != token->len   ||
        memcmp(token->str, op, token->len)) {
        error_at(token->str, "'%s'ではありません", op);
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

        if (startswith("==", p) ||
            startswith("!=", p) ||
            startswith(">=", p) ||
            startswith("<=", p) ) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        // カンマと代入演算子は1文字演算子と同様に予約語として扱う
        if (strchr("+-*/()<>;=", *p) != NULL) {
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

        if ('a' <= *p && *p <= 'z') {
            cur = new_token(TK_IDENT, cur, p++, 1);
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

// program = stmt*
void program(void) {
    int i;

    for (i = 0; !at_eof() && i < MAX_LINES; i++) {
        code[i] = stmt();
    }
    code[i] = NULL;
}

// stmt = expr ";"
Node  *stmt(void) {
    Node *node = expr();
    expect(";");
    return node;
}

// expr = assign
Node *expr(void) {
    return assign();
}

// assign = equality ("=" assign)?
Node *assign(void) {
    Node *node = equality();
    if (consume("=")) {
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality(void) {
    Node *node = relational();
    for(;;) {
        if (consume("==")) {
            node = new_node(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_node(ND_NE, node, relational());
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
            node = new_node(ND_LT, node, add());
        } else if (consume("<=")) {
            node = new_node(ND_LE, node, add());
        } else if (consume(">")) {
            node = new_node(ND_LT, add(), node);    // ">"は"<"の左右を入れ換えたもの
        } else if (consume(">=")) {
            node = new_node(ND_LE, add(), node); // ">="もまた同様とみなす
        } else {
            return node;
        }
    }
}

// add = mul ("+" mul | "-" mul)*
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

// primary = num | ident | "(" expr ")"
Node *primary(void) {
    // 括弧で囲まれている場合
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    // 識別子に対してスタックフレーム内の位置を予め割りてておく
    Token *token = consume_ident();
    if (token != NULL) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;
        node->offset = (token->str[0] - 'a' + 1) * 8;
        return node;
    }

    // そうでなければ数値でなければならない
    return new_node_num(expect_number());
}

