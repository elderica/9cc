/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
#include "9cc.h"

static void gen_lval(Node *node);
static void gen(Node *node);

static void gen_lval(Node *node) {
    if (node->kind != ND_LVAR) {
        error("代入の左辺値が変数ではありません");
    }

    printf("  # gen_lval\n");
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
}

void gen(Node *node) {
    switch (node->kind) {
        case ND_NUM:
            printf("  # ND_NUM\n");
            printf("  push %d\n", node->val);
            return;
        case ND_LVAR:
            printf("  # ND_LVAR\n");
            gen_lval(node);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            return;
        case ND_ASSIGN:
            printf("  # ND_ASSIGN\n");
            gen_lval(node->lhs);
            gen(node->rhs);

            printf("  pop rdi\n");
            printf("  pop rax\n");
            printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
            return;
        case ND_RETURN:
            // gencode関数により余計な処理や後続の式に対するアセンブラコードも出力される
            // しかし、ここでretするので、無視してよい
            gen(node->lhs);
            printf("  # ND_RETURN\n");
            printf("  pop rax\n");
            printf("  mov rsp, rbp\n");
            printf("  pop rbp\n");
            printf("  ret\n");
            return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  # %s:%d\n", __FILE__, __LINE__);
    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
        case ND_ADD:
            printf("  # ND_ADD\n");
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  # ND_SUB\n");
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  # ND_MUL\n");
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  # ND_NIV\n");
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
            printf("  # ND_EQ\n");
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NE:
            printf("  # ND_NE\n");
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LT:
            printf("  # ND_LT\n");
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LE:
            printf("  # ND_LE\n");
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
    }

    printf("  push rax\n");
}

// コード生成器のエントリポイント
void gencode(void) {
    // プロローグを出力する
    // 変数のための領域を確保する
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n"); // a〜zの26個の変数×8バイト

    for (int i = 0; code[i] != NULL; i++) {
        printf("  # %s:%d i:%d\n", __FILE__, __LINE__, i);
        gen(code[i]);
    }

    // エピローグ
    // 最後の式の結果がraxに残り、返り値となる。
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}
