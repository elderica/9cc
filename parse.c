/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include "9cc.h"

// 現在着目しているトークン
Token *token;

// 入力プログラム
char *user_input;

// ローカル変数
LVar *locals = NULL;

static void error_at(char *loc, char *fmt, ...);
static Token *new_token(TokenKind kind, Token *cur, char *str, int len);
static char *starts_with_reserved(char *p);

static Node *new_node(NodeKind kind);
static Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs);
static Node *new_node_num(int val);
static Node *new_node_lvar(LVar *var);
static Node *new_node_if(Node *cond, Node *then, Node *els);
static Node *new_node_while(Node *cond, Node *body);

static bool consume(char *symbol);
static Token *consume_ident(void);
static void expect(char *symbol);
static int expect_number(void);
static bool at_eof(void);

static LVar *find_lvar(Token *tok);
static LVar *new_lvar(Token *tok);

static bool is_alnum(char c);

static Function *new_function(Node *nodes, LVar *locals);

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

static char *starts_with_reserved(char *p) {
    static char *keywords[] = {
        "return",
        "if",
        "else",
        "while",
        "for",
    };
    for (int i = 0; i < (sizeof(keywords) / sizeof(keywords[0])); i++) {
        int l = strlen(keywords[i]);
        if (startswith(keywords[i], p) && !is_alnum(p[l])) {
            return keywords[i];
        }
    }

    static char *multi_chars_operators[] = {
        "==",
        "!=",
        "<=",
        ">=",
    };
    for (int i = 0; i < (sizeof(multi_chars_operators) / sizeof(multi_chars_operators[0])); i++) {
        if (startswith(multi_chars_operators[i], p)) {
            return multi_chars_operators[i];
        }
    }

    return NULL;
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

        char *symbol = starts_with_reserved(p);
        if (symbol != NULL) {
            int l = strlen(symbol);
            cur = new_token(TK_RESERVED, cur, p, l);
            p += l;
            continue;
        }

        // カンマと代入演算子は1文字演算子と同様に予約語として扱う
        if (strchr("+-*/()<>;={}", *p) != NULL) {
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

static Node *new_node(NodeKind kind) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

static Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
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

static Node *new_node_if(Node *cond, Node *then, Node *els) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_IF;

    node->cond = cond;
    node->then = then;
    node->els = els;

    return node;
}

static Node *new_node_while(Node *cond, Node *body) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_WHILE;

    node->cond = cond;
    node->body = body;

    return node;
}

static Node *new_node_for(Node *init, Node *cond, Node *inc, Node *body) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_FOR;

    node->init = init;
    node->cond = cond;
    node->inc  = inc;
    node->body = body;

    return node;
}

// 識別子として許す文字種を定義する
static bool is_alnum(char c) {
    return  isalnum(c) || (c == '_');
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

Function *new_function(Node *nodes, LVar *locals) {
    Function *func = calloc(1, sizeof(Function));
    func->nodes = nodes;
    func->locals = locals;

    return func;
}
// program = stmt*
Function* program(void) {
    // 変数の新たな有効範囲を導入する。
    locals = NULL;

    Node dummy;
    Node *cur = &dummy;

    while (!at_eof()){
        cur->next = stmt();
        cur = cur->next;
    }
    cur->next = NULL;

    return new_function(dummy.next, locals);
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | expr ";"
static Node  *stmt(void) {
    Node *node;

    if (consume("return")) {
        node = new_node_binary(ND_RETURN, expr(), NULL);
        expect(";");
        return node;
    }

    if (consume("if")) {
        expect("(");
        Node *cond = expr();
        expect(")");
        Node *then = stmt();
        Node *els = NULL;
        if (consume("else")) {
            els = stmt();
        }
        node = new_node_if(cond, then, els);
        return node;
    }

    if (consume("while")) {
        expect("(");
        Node *cond = expr();
        expect(")");
        Node *body = stmt();
        node = new_node_while(cond, body);
        return node;
    }

    if (consume("for")) {
        expect("(");
        Node *init = NULL;
        if (!consume(";")) {
            init = new_node_binary(ND_EXPR_STMT, expr(), NULL);
            expect(";");
        }
        Node *cond = NULL;
        if (!consume(";")) {
            cond = expr();
            expect(";");
        }
        Node *inc = NULL;
        if (!consume(")")) {
            inc = new_node_binary(ND_EXPR_STMT, expr(), NULL);
            expect(")");
        }
        Node *body = stmt();
        return new_node_for(init, cond, inc, body);
    }

    if (consume("{")) {
        Node head = {};
        Node *cur = &head;

        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }
        cur->next = NULL;

        Node *node = new_node(ND_BLOCK);
        node->block = head.next;

        return node;
    }

    node = new_node_binary(ND_EXPR_STMT, expr(), NULL);
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
        node = new_node_binary(ND_ASSIGN, node, assign());
    }
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
    Node *node = relational();
    for(;;) {
        if (consume("==")) {
            node = new_node_binary(ND_EQ, node, relational());
        } else if (consume("!=")) {
            node = new_node_binary(ND_NE, node, relational());
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
            node = new_node_binary(ND_LT, node, add());
        } else if (consume("<=")) {
            node = new_node_binary(ND_LE, node, add());
        } else if (consume(">")) {
            node = new_node_binary(ND_LT, add(), node);    // ">"は"<"の左右を入れ換えたもの
        } else if (consume(">=")) {
            node = new_node_binary(ND_LE, add(), node); // ">="もまた同様とみなす
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
            node = new_node_binary(ND_ADD, node, mul());
        } else if (consume("-")) {
            node = new_node_binary(ND_SUB, node, mul());
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
            node = new_node_binary(ND_MUL, node, unary());
        } else if(consume("/")) {
            node = new_node_binary(ND_DIV, node, unary());
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
        return new_node_binary(ND_SUB, new_node_num(0), unary());
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

