/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include "9cc.h"

// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

// 各行(式の並び)の構文解析結果を保存する配列
Node *code[MAX_LINES+1];

// ローカル変数
LVar *locals = NULL;

static void error_at(char *loc, char *fmt, ...);
static Token *new_token(TokenKind kind, Token *cur, char *str, int len);

static Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
static Node *new_node_num(int val);
static Node *new_node_lvar(LVar *var);

static bool consume(char *symbol);
static Token *consume_ident(void);
static void expect(char *symbol);
static int expect_number(void);
static bool at_eof(void);

static LVar *find_lvar(Token *tok);
static LVar *new_lvar(Token *tok);

static bool is_alnum(char c);

static Node *stmt(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);


// プログラム中のどこにエラーがあるか報告する
static void error_at(char *loc, char *fmt, ...) {
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
static bool consume(char *symbol) {
    if (token->kind != TK_RESERVED ||
        strlen(symbol) != token->len   ||
        memcmp(token->str, symbol, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

// 次のトークンが識別子ならば、そのトークンを返してトークンを一つ読み進める。
// そうでなければNULLを返す。
static Token *consume_ident(void) {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *cur = token;
    token = token->next;
    return cur;
}

// 次のトークンが、期待している記号ならば、トークンを1つ読み進める。
// それ以外のときは、エラーを報告する。
static void expect(char *symbol) {
    if (token->kind != TK_RESERVED ||
        strlen(symbol) != token->len   ||
        memcmp(token->str, symbol, token->len)) {
        error_at(token->str, "'%s'ではありません", symbol);
    }
    token = token->next;
}

// 次のトークンが数値ならば、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
static int expect_number() {
    if (token->kind != TK_NUM) {
        error_at(token->str, "数ではありません");
    }
    int val = token->val;
    token = token->next;
    return val;
}

static bool at_eof(void) {
    return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
// 新しく最後尾になった新しいトークンを返す。
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
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

        if (startswith("return", p) && !is_alnum(p[6])) {
            cur = new_token(TK_RESERVED, cur, p, 6);
            p += 6;
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

        if (is_alnum(*p)) {
            // 識別子の長さを計算する
            char *q = p;
            while (is_alnum(*q)) {
                q++;
            }
            cur = new_token(TK_IDENT, cur, p, q-p);
            p = q;
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

static Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

static Node *new_node_lvar(LVar *var) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->var = var;

    return node;
}

// 識別子として許す文字種を定義する
static bool is_alnum(char c) {
    return  ('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z') ||
            ('0' <= c && c <= '9') ||
            (c == '_');
}

// 変数を名前で検索する。無ければNULLを返す。
static LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var != NULL; var = var->next) {
        if (var->len == tok->len && memcmp(tok->str, var->name, var->len) == 0) {
            return var;
        }
    }
    return NULL;
}

static LVar *new_lvar(Token *tok) {
    LVar *var = calloc(1, sizeof(LVar));
    var->name = tok->str;
    var->len = tok->len;

    var->next = locals;
    locals = var;
    return var;
}

// program = stmt*
void program(void) {
    int i;

    for (i = 0; !at_eof() && i < MAX_LINES; i++) {
        code[i] = stmt();
    }
    code[i] = NULL;

    // 変数にオフセットを割り当てる
    int o = 0;
    for (LVar *var = locals; var != NULL; var = var->next) {
        var->offset = o;
        o += 8;
    }
}

// stmt = expr ";" | "return" expr ";"
static Node  *stmt(void) {
    Node *node;

    if (consume("return")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    } else {
        node = expr();
    }
    expect(";");
    return node;
}

// expr = assign
static Node *expr(void) {
    return assign();
}

// assign = equality ("=" assign)?
static Node *assign(void) {
    Node *node = equality();
    if (consume("=")) {
        node = new_node(ND_ASSIGN, node, assign());
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
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
static Node *relational(void) {
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
static Node *add(void) {
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
static Node *mul(void) {
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
static Node *unary(void) {
    if (consume("+")) {
        return unary();
    }
    if (consume("-")) {
        return new_node(ND_SUB, new_node_num(0), unary());
    }
    return primary();
}

// primary = num | ident | "(" expr ")"
static Node *primary(void) {
    // 括弧で囲まれている場合
    if (consume("(")) {
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if (tok != NULL) {
        // 既出の変数ならばそれを参照させ、そうでなければ新規の変数として扱う
        LVar *lvar = find_lvar(tok);
        if (lvar == NULL) {
            lvar = new_lvar(tok);
        }
        return new_node_lvar(lvar);
    }

    // そうでなければ数値でなければならない
    return new_node_num(expect_number());
}

